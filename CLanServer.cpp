#include "SharedPCH.h"

long CLanServer::sTidSeed = 0;

CLanServer::~CLanServer()
{
	Stop();
}

bool CLanServer::Start(const char * addr,
                       int          port,
                       int          workThNum,
                       bool         nagleOnOff,
                       int          maxCon
)
{
	strcpy_s(mAddr, addr);
	mPort       = port;
	mWorkThNum  = workThNum;
	mNagleOnOff = nagleOnOff;
	mMaxCon     = maxCon;

	mSesses        = new CLanSess[mMaxCon];
	mWorkerThreads = new HANDLE[mWorkThNum];


	SOCKADDR_IN sockaddrIn;
	int         ret;

	miocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	if (miocp == NULL)
	{
		LPSTR errMsg = nullptr;
		int   err    = GetLastError();
		formatMsg(err, &errMsg);
		OnError(err, errMsg);
		LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);

		LocalFree(errMsg);
		return false;
	}

	mListenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (mListenSock == INVALID_SOCKET)
	{
		LPSTR errMsg = nullptr;
		int   err    = WSAGetLastError();
		formatMsg(err, &errMsg);
		OnError(err, errMsg);
		LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);

		LocalFree(errMsg);
		return false;
	}

	memset(&sockaddrIn, 0, sizeof(sockaddrIn));
	sockaddrIn.sin_family = AF_INET;
	sockaddrIn.sin_port   = htons(port);
	// sockaddrIn.sin_addr.s_addr = htonl(INADDR_ANY);
	InetPtonA(AF_INET, addr, &sockaddrIn.sin_addr);

	ret = bind(mListenSock, (SOCKADDR*)&sockaddrIn, sizeof(sockaddrIn));
	if (ret == SOCKET_ERROR)
	{
		LPSTR errMsg = nullptr;
		int   err    = WSAGetLastError();
		formatMsg(err, &errMsg);
		OnError(err, errMsg);

		LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);

		LocalFree(errMsg);
		return false;
	}

	// SocketOpt///////////////////////////
	if (!mNagleOnOff)
	{
		bool optval = true;
		int  ret;
		ret = setsockopt(mListenSock,
		                 IPPROTO_TCP,
		                 TCP_NODELAY,
		                 (char*)&optval,
		                 sizeof(optval));
		if (ret == SOCKET_ERROR)
		{
			LPSTR errMsg = nullptr;
			int   err    = WSAGetLastError();
			formatMsg(err, &errMsg);
			OnError(err, errMsg);
			LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);

			LocalFree(errMsg);

			return false;
		}
	}
	// //////////////////////////////SocketOpt

	ret = listen(mListenSock, SOMAXCONN);
	if (ret == SOCKET_ERROR)
	{
		LPSTR errMsg = nullptr;
		int   err    = WSAGetLastError();
		formatMsg(err, &errMsg);
		OnError(err, errMsg);

		LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);

		LocalFree(errMsg);
		return false;
	}

	mUpdateTh = (HANDLE)_beginthreadex(0,
	                                   0,
	                                   UpdateThread,
	                                   this,
	                                   0,
	                                   0);
	mMonitorTh = (HANDLE)_beginthreadex(0,
	                                    0,
	                                    MonitorThread,
	                                    this,
	                                    0,
	                                    0);


	for (int i = 0; i < mWorkThNum; ++i)
	{
		mWorkerThreads[i] = (HANDLE)_beginthreadex(0,
		                                           0,
		                                           WorkerThread,
		                                           this,
		                                           0,
		                                           0);
	}

	LOG("Engine",dfLOG_LEVEL_SYSTEM,
		"LanServer Started__addr: %s port: %d workThNum : %d nagleOnOff : %d  maxCon: %d"
	,mAddr,mPort,mWorkThNum,mNagleOnOff,mMaxCon);
	return true;
}

void CLanServer::Stop()
{

	// Kill Threads
	if (mWorkerThreads)
	{
		for (int i = 0; i < mWorkThNum; ++i)
		{
			PostQueuedCompletionStatus(miocp, 0, 0, 0);
		}
		mShutdown = true;

		// Wait Threads
		HANDLE * ths = new HANDLE[mWorkThNum + 2];
		ths[0]       = mUpdateTh;
		ths[1]       = mMonitorTh;
		for (int i   = 0; i < mWorkThNum; ++i)
		{
			ths[i + 2] = mWorkerThreads[i];
		}
		WaitForMultipleObjects(mWorkThNum + 2, ths, true, INFINITE);
		delete[] ths;
		mWorkerThreads = nullptr;
	}

	if (mSesses)
	{
		delete[] mSesses;
		mSesses = nullptr;
	}
}

int CLanServer::GetClientCount()
{
	return mSessCnt;
}

bool CLanServer::Disconnect(uint32_t sessid)
{
	if (sessid >= mMaxCon)
	{
		char errMsg[100];
		sprintf_s(errMsg, "Wrong Sessid");
		OnError(UtilDef::Network::Error::ENGINE_ERROR, errMsg);
		LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);
	}
	Disconnect(&mSesses[sessid]);
	return false;
}

bool CLanServer::SendPacket(uint32_t sessid, CSeriesBuffer * packet)
{
	packet->IncreRef();
	PRO_BEGIN(__FUNCTION__);

	if (sessid >= mMaxCon)
	{
		char errMsg[100];
		sprintf_s(errMsg, "Wrong Sessid");
		OnError(UtilDef::Network::Error::ENGINE_ERROR, errMsg);
		LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);
	}

	int        ret  = 0;
	CLanSess * sess = nullptr;
	sess            = &mSesses[sessid];

	uint16_t len = packet->GetDataSize();
	packet->PutHead((char*)&len);

	ret = sess->sendQ.Enqueue((char*)&packet, sizeof(CSeriesBuffer*));
	if (ret != sizeof(CSeriesBuffer*))
	{
		char errMsg[100];
		sprintf_s(errMsg, "Enqueue Fail");
		OnError(UtilDef::Network::Error::ENGINE_ERROR, errMsg);
		LOG("Engine",dfLOG_LEVEL_ERROR, "%s",errMsg);
		return false;
	}

	OND_SESS(
		sess->SESS_HISTORY("");
	)

	SendPost(sess);
	PRO_END(__FUNCTION__);
	return true;
}

int32_t CLanServer::GenSessId()
{
	uint32_t ret = -1;
	if (mSessCnt == mMaxCon)
	{
		char errMsg[100];
		sprintf_s(errMsg, "Over Maxcon");
		OnError(UtilDef::Network::Error::ENGINE_ERROR, errMsg);
		LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);
		return -1;
	}
	InterlockedIncrement((long*)&mSessCnt);
	for (uint32_t i = 0; i < mMaxCon; ++i)
	{
		if (!mSesses[i].isValid)
		{
			ret = i;
			break;
		}
	}
	return ret;
}

bool CLanServer::Update()
{
	SOCKET      newSock;
	SOCKADDR_IN sockaddrIn;
	int         sockaddrSize = sizeof(sockaddrIn);

	newSock = accept(mListenSock, (SOCKADDR*)&sockaddrIn, &sockaddrSize);
	if (newSock == INVALID_SOCKET)
	{
		LPSTR errMsg = nullptr;
		int   err    = WSAGetLastError();
		formatMsg(err, &errMsg);
		OnError(err, errMsg);

		LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);

		LocalFree(errMsg);

		return false;
	}
	PRO_BEGIN("AfterAccept");
	char addr[20];
	int  port = ntohs(sockaddrIn.sin_port);
	InetNtopA(AF_INET, &sockaddrIn.sin_addr, addr, 20);

	///// Call Virtual Method////////////////////////
	if (!OnConnectionRequest(addr, port))
	{
		return false;
	}

	// SocketOpt///////////////////////////
	if (!mNagleOnOff)
	{
		bool optval = true;
		int  ret;
		ret = setsockopt(newSock,
		                 IPPROTO_TCP,
		                 TCP_NODELAY,
		                 (char*)&optval,
		                 sizeof(optval));
		if (ret == SOCKET_ERROR)
		{
			LPSTR errMsg = nullptr;
			int   err    = WSAGetLastError();
			formatMsg(err, &errMsg);
			OnError(err, errMsg);
			LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);

			LocalFree(errMsg);

			return false;
		}
		int sndbufSz = 0;
		ret          = setsockopt(newSock,
		                          SOL_SOCKET,
		                          SO_SNDBUF,
		                          (char*)&sndbufSz,
		                          sizeof(sndbufSz));
		if (ret == SOCKET_ERROR)
		{
			LPSTR errMsg = nullptr;
			int   err    = WSAGetLastError();
			formatMsg(err, &errMsg);
			OnError(err, errMsg);
			LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);

			LocalFree(errMsg);

			return false;
		}
		int rcvbufSz = 0;
		ret          = setsockopt(newSock,
		                          SOL_SOCKET,
		                          SO_RCVBUF,
		                          (char*)&rcvbufSz,
		                          sizeof(rcvbufSz));
		if (ret == SOCKET_ERROR)
		{
			LPSTR errMsg = nullptr;
			int   err    = WSAGetLastError();
			formatMsg(err, &errMsg);
			OnError(err, errMsg);
			LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);

			LocalFree(errMsg);

			return false;
		}
	}
	// //////////////////////////////SocketOpt


	// 자료구조에 넣기
	int sessid = GenSessId();
	if (sessid == -1)
	{
		closesocket(newSock);
		return true;
	}

	CLanSess * newSess = &mSesses[sessid];

	newSess->Init(sessid,
	              newSock,
	              sockaddrIn,
	              addr,
	              port,
	              true);
	OND_SESS(
		newSess->SESS_HISTORY("Id %d", sessid);
	)



	mAcceptTps++;
	mAcceptTotal++;

	HANDLE retHandle = CreateIoCompletionPort((HANDLE)newSock,
	                                          miocp,
	                                          (ULONG_PTR)newSess,
	                                          0);
	if (retHandle == NULL)
	{
		LPSTR errMsg = nullptr;
		int   err    = WSAGetLastError();
		formatMsg(err, &errMsg);
		OnError(err, errMsg);
		LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);

		LocalFree(errMsg);

		return false;
	}


	//To prevent Releasing by sendIO vanishing
	//Increment ioCnt, then Decrement after RecvPost
	InterlockedIncrement((long*)&newSess->ioCnt);
	///// Call Virtual Method////////////////////////
	OnClientJoin(addr, port, newSess->sessId);

	// Print Info////////////////////////
	LOG("Engine",dfLOG_LEVEL_SYSTEM, "Accept: %s:%d", newSess->addr, newSess->
		port);

	RecvPost(newSess);

	int ret = InterlockedDecrement((long*)&newSess->ioCnt);
	if (ret == 0)
	{
		char errMsg[100];
		sprintf_s(errMsg, "Release in Update(Accept) %d", ret);
		OnError(UtilDef::Network::Error::ENGINE_ERROR, errMsg);
		LOG("Engine", dfLOG_LEVEL_SYSTEM, "%s", errMsg);

		Release(newSess);
	}


	PRO_END("AfterAccept");

	return true;
}

bool CLanServer::SendPost(CLanSess * sess)
{
	PRO_BEGIN(__FUNCTION__);
	int useSize = sess->sendQ.GetUseSize();
	if (useSize == 0) return false;
	if (InterlockedCompareExchange((LONG*)&sess->isSendable, 0, 1))
	{
		OND_SESS(
			sess->SESS_HISTORY("");
		)

		int   ret;
		DWORD bytesSend = 0;
		DWORD flags     = 0;

		WSABUF wsabuf[UtilDef::Network::SERVER_WSASEND_MAX];

		int i  = 0;
		for (i = 0; i < UtilDef::Network::SERVER_WSASEND_MAX; ++i)
		{
			CSeriesBuffer * csb = nullptr;
			ret                 = sess->sendQ.Dequeue((char*)&csb,
			                                          sizeof(CSeriesBuffer*));
			if (ret == 0)
			{
				break;
			}
			ret = sess->sendPendsQ.Enqueue((char*)&csb,
			                               sizeof(CSeriesBuffer*));
			wsabuf[i].len = csb->GetAllDataSize();
			wsabuf[i].buf = (char*)csb->GetHeadBufferPtr();
		}
		int bufCnt = i;
		if (bufCnt == 0)
		{
			sess->isSendable = true;
			return false;
		}

		memset(sess->sendOverlapped, 0, sizeof(_OVERLAPPED));

		InterlockedIncrement((long*)&sess->ioCnt);

		ret = WSASend(sess->sock,
		              wsabuf,
		              bufCnt,
		              &bytesSend,
		              flags,
		              sess->sendOverlapped,
		              nullptr);
		if (ret == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			if (error != WSA_IO_PENDING)
			{
				ret = InterlockedDecrement((long*)&sess->ioCnt);
				if (ret < 0)
				{
					char errMsg[100];
					sprintf_s(errMsg, "Io Count Error!! %d", ret);
					OnError(UtilDef::Network::Error::ENGINE_ERROR, errMsg);
					LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);
					return true;
				}
				if (ret == 0)
				{
					char errMsg[100];
					sprintf_s(errMsg, "Release in WSASend %d", ret);
					OnError(UtilDef::Network::Error::ENGINE_ERROR, errMsg);
					LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);

					Release(sess);
				}

				return true;
			}
		}
	}
	PRO_END(__FUNCTION__);
	return true;
}

bool CLanServer::RecvPost(CLanSess * sess)
{
	OND_SESS(
		sess->SESS_HISTORY("");
	)

	PRO_BEGIN(__FUNCTION__);

	CRingBuffer &  recvQ       = sess->recvQ;
	LPOVERLAPPED & recvOverlap = sess->recvOverlapped;

	int ret;

	WSABUF wsabuf[2];
	int    bufCnt       = 0;
	DWORD  numBytesRecv = 0;
	DWORD  flags        = 0;


	int freeSize         = recvQ.GetFreeSize();
	int notBrokenPutSize = recvQ.GetNotBrokenPutSize();
	if (freeSize == notBrokenPutSize)
	{
		bufCnt        = 1;
		wsabuf[0].len = notBrokenPutSize;
		wsabuf[0].buf = recvQ.GetRearBufferPtr();
	}
	else
	{
		bufCnt        = 2;
		wsabuf[0].len = notBrokenPutSize;
		wsabuf[0].buf = recvQ.GetRearBufferPtr();
		wsabuf[1].len = freeSize - notBrokenPutSize;
		wsabuf[1].buf = recvQ.GetBufferPtr();
	}
	memset(recvOverlap, 0, sizeof(_OVERLAPPED));

	InterlockedIncrement((long*)&sess->ioCnt);

	ret = WSARecv(sess->sock,
	              wsabuf,
	              bufCnt,
	              &numBytesRecv,
	              &flags,
	              recvOverlap,
	              nullptr);
	// CompletionRoutine);
	if (ret == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if (error != WSA_IO_PENDING)
		{
			int ret;
			ret = InterlockedDecrement((long*)&sess->ioCnt);
			if (ret < 0)
			{
				char errMsg[100];
				sprintf_s(errMsg, "Io Count Error!! %d", ret);
				OnError(UtilDef::Network::Error::ENGINE_ERROR, errMsg);
				LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);
				return true;
			}
			if (ret == 0)
			{
				char errMsg[100];
				sprintf_s(errMsg, "Release in WSARecv %d", ret);
				OnError(UtilDef::Network::Error::ENGINE_ERROR, errMsg);
				LOG("Engine", dfLOG_LEVEL_SYSTEM, "%s", errMsg);

				Release(sess);
			}

			return true;
		}
	}

	PRO_END(__FUNCTION__);

	return true;
}

void CLanServer::ProcRecv(CLanSess * sess)
{
	OND_SESS(
		sess->SESS_HISTORY("");
	)
	PRO_BEGIN(__FUNCTION__);
	CRingBuffer & recvQ = sess->recvQ;
	int           ret;
	int           dequeSize = 0;

	while (1)
	{
		dequeSize = recvQ.GetUseSize();
		if (dequeSize < dfPACKET_HEADER_SIZE) break;

		uint16_t len;
		ret = recvQ.Peek((char*)&len, dfPACKET_HEADER_SIZE);
		if (ret != dfPACKET_HEADER_SIZE)
		{
			char errMsg[100];
			sprintf_s(errMsg, "Wrong RingBuffer");
			OnError(UtilDef::Network::Error::ENGINE_ERROR, errMsg);
			LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);
			return;
		}
		if (dequeSize < dfPACKET_HEADER_SIZE + len) break;

		recvQ.MoveFront(sizeof(uint16_t));

		CSeriesBuffer * packet = CSeriesBuffer::Alloc();


		ret = recvQ.Dequeue((char*)packet->GetBufferPtr(),
		                    len);
		if (ret != len)
		{
			CSeriesBuffer::Free(packet);

			char errMsg[100];
			sprintf_s(errMsg, "Wrong RingBuffer");
			OnError(UtilDef::Network::Error::ENGINE_ERROR, errMsg);
			LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);
			return;
		}
		packet->PutHead((char*)&len);
		packet->MoveWritePos(ret);

		///// Call Virtual Method////////////////////////
		OnRecv(sess->sessId, packet);
		CSeriesBuffer::Free(packet);
	}

	RecvPost(sess);
	PRO_END(__FUNCTION__);
}

void CLanServer::Disconnect(CLanSess * sess)
{
	OND_SESS(
		sess->SESS_HISTORY("");
	)

	shutdown(sess->sock, SD_BOTH);
}

void CLanServer::Release(CLanSess * sess)
{
	OND_SESS(
		sess->SESS_HISTORY("");
	)

	PRO_BEGIN(__FUNCTION__);

	//Error Handle : Erase Session in array
	if (mSessCnt == 0)
	{
		char errMsg[100];
		sprintf_s(errMsg, "Over Release");
		OnError(UtilDef::Network::Error::ENGINE_ERROR, errMsg);
		LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);
		return;
	}

	///// Call Virtual Method////////////////////////
	OnClientLeave(sess->sessId);

	// Print Info////////////////////////
	LOG("Engine", dfLOG_LEVEL_SYSTEM, "Release: %s:%d", sess->addr, sess->port);

	closesocket(sess->sock);

	//Free Remain sends packets 
	int ret     = 0;
	int useSize = sess->sendQ.GetUseSize();
	if (useSize > 0)
	{
		while (1)
		{
			CSeriesBuffer * pendPac = nullptr;

			ret = sess->sendQ.Dequeue((char*)&pendPac,
			                          sizeof(CSeriesBuffer*));
			if (ret == 0)break;

			CSeriesBuffer::Free(pendPac);
		}
	}
	//Free pending Packets
	ret     = 0;
	useSize = sess->sendPendsQ.GetUseSize();
	if (useSize > 0)
	{
		while (1)
		{
			CSeriesBuffer * pendPac = nullptr;

			ret = sess->sendPendsQ.Dequeue((char*)&pendPac,
			                               sizeof(CSeriesBuffer*));
			if (ret == 0)break;

			CSeriesBuffer::Free(pendPac);
		}
	}

	mSesses[sess->sessId].isValid = false;

	InterlockedDecrement((long*)&mSessCnt);
	mReleaseTotal++;

	PRO_END(__FUNCTION__);
}


unsigned CLanServer::UpdateThread(void * pParam)
{
	auto thisP = (CLanServer*)pParam;
	while (1)
	{
		if (thisP->IsShutdown()) break;
		thisP->Update();
	}
	return 0;
}

unsigned CLanServer::MonitorThread(void * pParam)
{
	auto thisP = (CLanServer*)pParam;
	// static DWORD befTime = GetTickCount();
	while (1)
	{
		if (thisP->IsShutdown()) break;
		Sleep(1000);

		thisP->ResetMonitorVal();
	}
	return 0;
}

unsigned __stdcall CLanServer::WorkerThread(void * pParam)
{
	CLanServer * thisP = (CLanServer*)pParam;
	int          id    = GetCurrentThreadId();
	int          tid   = InterlockedIncrement(&thisP->sTidSeed);

	while (1)
	{
		bool         ret;
		DWORD        transfered = 0;
		ULONG_PTR    key        = 0;
		LPOVERLAPPED overlapped = nullptr;

		ret = GetQueuedCompletionStatus(thisP->miocp,
		                                &transfered,
		                                &key,
		                                &overlapped,
		                                INFINITE);

		CLanSess * sess = (CLanSess*)key;

		///// Call Virtual Method////////////////////////
		thisP->OnWorkerThreadBegin();


		if (transfered == 0 && key == 0 && overlapped == 0)
		{
			LOG("Engine",dfLOG_LEVEL_SYSTEM,
				"GetQueuedCompletionStatus() : Thread Shutdown");
			break;
		}
		if (transfered == 0 && ret == false)
		{
			thisP->Disconnect(sess);
		}

		if (overlapped == sess->recvOverlapped)
		{
			sess->recvQ.MoveRear(transfered);
			thisP->ProcRecv(sess);
		}
		else if (overlapped == sess->sendOverlapped)
		{
			uint32_t useSize = sess->sendPendsQ.GetUseSize();

			int i = 0;
			ret   = 0;
			if (useSize > 0)
			{
				PRO_BEGIN("Dequeue Sendpends");

				while (1)
				{
					CSeriesBuffer * pendPac = nullptr;

					ret = sess->sendPendsQ.Dequeue((char*)&pendPac,
					                               sizeof(CSeriesBuffer*));
					if (ret == 0)break;

					///// Call Virtual Method////////////////////////
					thisP->OnSend(sess->sessId,
					              pendPac->GetDataSize());

					CSeriesBuffer::Free(pendPac);
				}
				thisP->IncSendPacketTps(i);
				PRO_END("Dequeue Sendpends");
			}

			sess->isSendable = 1;
			// Caution! If useSize==0,  iocnt isn't incremented ( Isn't posted)
			thisP->SendPost(sess);
		}


		int retCnt;
		retCnt = InterlockedDecrement((long*)&sess->ioCnt);
		if (retCnt < 0)
		{
			char errMsg[100];
			sprintf_s(errMsg, "IO count Error %d", retCnt);
			thisP->OnError(UtilDef::Network::Error::ENGINE_ERROR, errMsg);
			LOG("Engine", dfLOG_LEVEL_ERROR, "%s", errMsg);
		}
		if (retCnt == 0)
		{
			thisP->Release(sess);
		}

		///// Call Virtual Method////////////////////////
		thisP->OnWorkerThreadEnd();
	}

	return 0;
}

void CLanServer::ResetMonitorVal()
{
	mRecentAcceptTps     = InterlockedExchange(&mAcceptTps, 0);
	mAcceptTps           = 0;
	mRecentRecvPacketTps = InterlockedExchange(&mRecvPacketTps, 0);
	mRecentSendPacketTps = InterlockedExchange(&mSendPacketTps, 0);
}
