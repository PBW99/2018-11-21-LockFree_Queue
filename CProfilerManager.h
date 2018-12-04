#pragma once

#define dfPROFILE_FUNCNUM_MAX 100

typedef struct CProfilers
{
	bool      isValid = false;
	CProfiler cprofiles[dfPROFILE_FUNCNUM_MAX];
	int       cprofilesIncnt = 1;
	int       tid;
}             CProfilers;

typedef struct ReducCProfilers
{
	ReducCProfiler cprofiles[dfPROFILE_FUNCNUM_MAX];
	int            cprofilesIncnt = 1;
	int            tid            = 0;
}                  ReducCProfilers;


class CProfilerManager
{
public:

	static CProfilerManager* GetCProfilerManager(int threadNum)
	{
		static CProfilerManager ins(threadNum);
		return &ins;
	}

	static void Setup(bool onoff, const char * fileDir, int threadNum);

	void BeginTag(const char * tagName);
	bool EndTag(const char *   tagName);

	/**
	 * \brief  Not Thread Safe, must be called by main thread
	 */
	void OutSummaryToFile();

	static uint64_t sfreq;
	static char     sFileDir[50];
	static bool     sOnOff;

	SRWLOCK      mCprofsSrwlock = SRWLOCK_INIT;
	CProfilers * mCprofiles;
	int          mCprofileIncnt = 1;
	int          mTlsIdx;


private:
	CProfilerManager(int threadNum)
	{
		mCprofiles = new CProfilers[threadNum];

		LARGE_INTEGER freqLI;
		QueryPerformanceFrequency(&freqLI);
		sfreq = freqLI.QuadPart;

		mTlsIdx = TlsAlloc();
	};

	virtual ~CProfilerManager()
	{
		delete[] mCprofiles;
	};
};

extern CProfilerManager * g_cProfileM;

#define PRO_BEGIN(...) g_cProfileM->BeginTag(__VA_ARGS__)
#define PRO_END(...) g_cProfileM->EndTag(__VA_ARGS__)
#define PRO_SUMMARY() g_cProfileM->OutSummaryToFile()
