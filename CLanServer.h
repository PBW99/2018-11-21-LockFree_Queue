#pragma once

#define dfPACKET_HEADER_SIZE 2

class CLanServer
{
public:

	        CLanServer() = default;
	virtual ~CLanServer();

	bool Start(const char * addr,
	           int          port,
	           int          workThNum,
	           bool         nagleOnOff,
	           int          maxCon
	);
	void Stop();
	int  GetClientCount();

	bool Disconnect(uint32_t sessid);
	bool SendPacket(uint32_t sessid, CSeriesBuffer * packet);

	// Ager Accept
	virtual bool OnConnectionRequest(char * addr, int port) = 0;
	// After Pass Connection 
	virtual void OnClientJoin(char * addr, int port, uint32_t sessid) = 0;
	// After Release
	virtual void OnClientLeave(uint32_t sessid) = 0;

	virtual void OnRecv(uint32_t sessid, CSeriesBuffer * packet) = 0;
	virtual void OnSend(uint32_t sessid, int             sendsize) = 0;
	virtual void OnWorkerThreadBegin() = 0;
	virtual void OnWorkerThreadEnd() = 0;
	virtual void OnError(int errorcode, char *) = 0;


	int GetMaxCon() const
	{
		return mMaxCon;
	}


	//Public Monitoring
	long mAcceptTotal  = 0;
	long mReleaseTotal = 0;
private:
	//////////////////Network/////////////////////////////
	char        mAddr[20];
	int         mPort;
	int         mWorkThNum;
	bool        mNagleOnOff;
	int         mMaxCon;
	CLanSess* mSesses;
	int mSessesMaxCnt=1;

	HANDLE miocp = 0;
	SOCKET mListenSock;

	int32_t GenSessId();

	bool Update();
	bool SendPost(CLanSess *   sess);
	bool RecvPost(CLanSess *   sess);
	void ProcRecv(CLanSess *   sess);
	void Disconnect(CLanSess * sess);
	/*
	 *오직 하나의 스레드만 Release를 호출
	 */
	void Release(CLanSess * sess);
	///////////////////////////////////////////////////////


	//////////////////Threads/////////////////////////////
	HANDLE      mUpdateTh  = 0;
	HANDLE      mMonitorTh = 0;
	HANDLE *    mWorkerThreads;
	static long sTidSeed;
	bool        mShutdown = 0;

	bool IsShutdown()
	{
		return mShutdown;
	}

	static unsigned __stdcall UpdateThread(void *  pParam);
	static unsigned __stdcall MonitorThread(void * pParam);
	static unsigned __stdcall WorkerThread(void *  pParam);
	///////////////////////////////////////////////////////


	//////////////////Monitoring/////////////////////////////
	long mSessCnt             = 0;
	long mAcceptTps           = 0;
	long mRecentAcceptTps     = 0;
	long mRecvPacketTps       = 0;
	long mSendPacketTps       = 0;
	long mRecentRecvPacketTps = 0;
	long mRecentSendPacketTps = 0;
	long mRecvTotal           = 0;
	long mSendTotal           = 0;

	void IncSendPacketTps(int a)
	{
		InterlockedAdd((long*)&mSendPacketTps, a);
		InterlockedAdd((long*)&mSendTotal, a);
	}

	void ResetMonitorVal();
	///////////////////////////////////////////////
};
