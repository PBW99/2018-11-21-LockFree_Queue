#pragma once
// #define DEBUG_SESS
#ifdef DEBUG_SESS
#define OND_SESS(...) __VA_ARGS__;
#define SESS_HISTORY(...) WriteHistory(__FUNCTION__, __VA_ARGS__);
#else
#define OND_SESS(...) ;
#endif


typedef struct CLanSess
{
	bool        isValid = false;
	UINT32      sessId;
	SOCKET      sock;
	SOCKADDR_IN sockaddrIn;
	char        addr[20];
	int         port;
	CRingBuffer recvQ =
		CRingBuffer(UtilDef::Network::SERVER_RINGBUFFER_SIZE);
	CRingBuffer sendQ =
		CRingBuffer(UtilDef::Network::SERVER_RINGBUFFER_SIZE);

	CRingBuffer sendPendsQ =
		CRingBuffer(UtilDef::Network::SERVER_RINGBUFFER_SIZE);
	LPOVERLAPPED recvOverlapped = nullptr;
	LPOVERLAPPED sendOverlapped = nullptr;
	int          isSendable;
	int          ioCnt = 0;
	OND_SESS(
		SRWLOCK sessLogLock = SRWLOCK_INIT;
		queue<string> sessLog;
	)


	CLanSess(): sessId(0),
	            sock(0),
	            sockaddrIn(),
	            addr{},
	            port(0),
	            isSendable(0)
	{
		recvOverlapped = new OVERLAPPED();
		sendOverlapped = new OVERLAPPED();
	}

	void Init(UINT32      _sessId,
	          SOCKET      _sock,
	          SOCKADDR_IN _sockaddrIn,
	          char *      _addr,
	          int         _port,
	          int         _isSendable)
	{
		isValid    = true;
		sessId     = _sessId;
		sock       = _sock;
		sockaddrIn = _sockaddrIn;
		isSendable = _isSendable;
		strcpy_s(addr, _addr);
		port = _port;
		new(recvOverlapped) OVERLAPPED();
		new(sendOverlapped) OVERLAPPED();
	}

	virtual ~CLanSess()
	{
		delete recvOverlapped;
		delete sendOverlapped;
	}

	OND_SESS(
		void WriteHistory(const char * funcName,
			const char * fmt,
			...)
		{
			char tmpc[50];
			snprintf(tmpc, 50, "%s/", funcName);

			char tmpc2[50];
			va_list ap;    // 가변 인자 목록 포인터
			va_start(ap, fmt);
			vsnprintf(tmpc2, 50, fmt, ap);
			va_end(ap);

			string tmp = tmpc;
			tmp += tmpc2;

			AcquireSRWLockExclusive(&sessLogLock);
			sessLog.emplace(tmp);
			if (sessLog.size() > 100) sessLog.pop();

			ReleaseSRWLockExclusive(&sessLogLock);
		}
	)
} CLanSess;
