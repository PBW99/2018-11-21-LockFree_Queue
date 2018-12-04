#pragma once
class UtilCrashDump
{
public:
	static UtilCrashDump* GetIns()
	{
		static UtilCrashDump ins;
		return &ins;
	}


	static void Crash(void)
	{
		int * p = nullptr;
		*p      = 0;
	}

	static LONG WINAPI MyExceptionFilter(PEXCEPTION_POINTERS pExceptionPointers)
	{
		int        workMem = 0;
		SYSTEMTIME stNowT;

		long DumpCOunt = InterlockedIncrement(&_DumpCount);

		// 현재 프로세스의 메모리 사용량
		HANDLE                  hProc = 0;
		PROCESS_MEMORY_COUNTERS pmc;

		hProc = GetCurrentProcess();

		if (NULL == hProc)
		{
			return 0;
		}
		if (GetProcessMemoryInfo(hProc, &pmc, sizeof(pmc)))
		{
			workMem = (int)(pmc.WorkingSetSize / 1024 / 1024);
		}
		CloseHandle(hProc);

		// 현재 날짜와 시간
		WCHAR filename[MAX_PATH];

		GetLocalTime(&stNowT);

		GetLocalTime(&stNowT);
		wsprintfW(filename,
		          L"Dump_%d%02d%02d_%02d.%02d.%02d_%d_%dMB.dmp",
		          stNowT.wYear,
		          stNowT.wMonth,
		          stNowT.wDay,
		          stNowT.wHour,
		          stNowT.wMinute,
		          stNowT.wSecond,
		          DumpCOunt,
		          workMem
		         );

		wprintf(L"\n\n\n!!! Crash Error !!! %d.%d.%d/%d:%d:%d\n",
		        stNowT.wYear,
		        stNowT.wMonth,
		        stNowT.wDay,
		        stNowT.wHour,
		        stNowT.wMinute,
		        stNowT.wSecond
		       );
		HANDLE hdumpfile = ::CreateFileW(filename,
		                                 GENERIC_WRITE,
		                                 FILE_SHARE_WRITE,
		                                 NULL,
		                                 CREATE_ALWAYS,
		                                 FILE_ATTRIBUTE_NORMAL,
		                                 NULL);

		if (hdumpfile != INVALID_HANDLE_VALUE)
		{
			_MINIDUMP_EXCEPTION_INFORMATION minidumpExceptionInformation;

			minidumpExceptionInformation.ThreadId = ::
				GetCurrentThreadId();
			minidumpExceptionInformation.ExceptionPointers = pExceptionPointers;
			minidumpExceptionInformation.ClientPointers    = TRUE;
			MiniDumpWriteDump(GetCurrentProcess(),
			                  GetCurrentProcessId(),
			                  hdumpfile,
			                  MiniDumpWithFullMemory,
			                  &minidumpExceptionInformation,
			                  NULL,
			                  NULL);
			CloseHandle(hdumpfile);

			wprintf(L"UtilCrashDump Save Finish!");
		}
		return EXCEPTION_EXECUTE_HANDLER;
	}

	static void SetHandlerDump()
	{
		SetUnhandledExceptionFilter(MyExceptionFilter);
	}
	// invalid parameter handler
	static void myInvaldParameterHandler(const wchar_t * expr,
	                                     const wchar_t * func,
	                                     const wchar_t * file,
	                                     unsigned int    line,
	                                     uintptr_t       pReserved)
	{
		Crash();
	}

	static int _custom_Report_hook(int irepopsttype, char * msg, int * retval)
	{
		Crash();
		return true;
	}

	static void myPurecallHandler(void)
	{
		Crash();
	}

	static LONG _DumpCount;

private:

	UtilCrashDump()
	{
		_DumpCount = 0;
		_invalid_parameter_handler oldHandler, newHandler;


		newHandler = myInvaldParameterHandler;
		oldHandler = _set_invalid_parameter_handler(newHandler);
		_CrtSetReportMode(_CRT_WARN, 0);
		_CrtSetReportMode(_CRT_ASSERT, 0);
		_CrtSetReportMode(_CRT_ERROR, 0);

		_CrtSetReportMode(_custom_Report_hook);

		_set_purecall_handler(myPurecallHandler);
		SetHandlerDump();
	}
};
