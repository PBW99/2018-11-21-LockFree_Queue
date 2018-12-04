#include "SharedPCH.h"

bool   g_LogOX    = true;
bool   g_logOutOX = false;
int    g_logLevel = 0;
string g_logDirName;
wstring g_wlogDirName;

DWORD g_logTimeInSec = 0;
DWORD g_logKey       = 0;

unordered_map<int, const char*> g_logLevToStr = {
	{dfLOG_LEVEL_DEBUG, "DEBUG"},
	{dfLOG_LEVEL_WARNING, "WARNG"},
	{dfLOG_LEVEL_ERROR, "ERROR"},
	{dfLOG_LEVEL_SYSTEM, "SYSTM"},
};
unordered_map<int, const wchar_t*> g_logLevToStrW = {
	{dfLOG_LEVEL_DEBUG, L"DEBUG"},
	{dfLOG_LEVEL_WARNING, L"WARNG"},
	{dfLOG_LEVEL_ERROR, L"ERROR"},
	{dfLOG_LEVEL_SYSTEM, L"SYSTM"},
};

void SetupMacroLogger(bool outOX, int logLevel, const char * logDirName)
{
	setlocale(LC_ALL, "");
	g_logOutOX   = outOX;
	g_logLevel   = logLevel;
	g_logDirName = logDirName;
	g_wlogDirName.assign(g_logDirName.begin(),g_logDirName.end());
	CreateDirectoryA(g_logDirName.c_str(), NULL);
}


void ShowLogStatus()
{
	wcout << L"[Log Status]-----------------------------------\n"
		<< L"	LogOnOff: " << g_LogOX << L"\n"
		<< L"	LogWCoutOnOff: " << g_logOutOX << L"\n"
		<< L"	LogLevel(DEBUG/WARNING/ERROR)" << g_logLevel << L"\n"
		<< L"-----------------------------------------------\n";
}

void GenLogHead(OUT char *          buf,
                    OUT struct tm * tm,
                    const char *    fileName,
                    const char *    funcName,
                    const char *    type,
                    int             line,
                    int             level)
{
	///Type////
	///Time////
	///LEVEL////
	///Unique Key////

	time_t rawtime;

	time(&rawtime);
	localtime_s(tm, &rawtime);

	InterlockedIncrement(&g_logKey);

	sprintf_s(buf,
	          1024,
	          "%s,\t%04d%02d%02d-%02d:%02d:%02d,%s,%010d,%s:%s:%d,\t",
	          type,
	          tm->tm_year + 1900,
	          tm->tm_mon + 1,
	          tm->tm_mday,
	          tm->tm_hour,
	          tm->tm_min,
	          tm->tm_sec,
	          g_logLevToStr[level],
	          g_logKey,
	          fileName,
	          funcName,
	          line
	         );
}

void GenLogHeadW(OUT wchar_t *       buf,
                     OUT tm *        tm,
                     const wchar_t * fileName,
                     const wchar_t * funcName,
                     const wchar_t * type,
                     int             line,
                     int             level)
{
	///Type////
	///Time////
	///LEVEL////
	///Unique Key////

	time_t rawtime;

	time(&rawtime);
	localtime_s(tm, &rawtime);

	InterlockedIncrement(&g_logKey);

	swprintf_s(buf,
	           1024,
	           L"%s,\t%04d%02d%02d-%02d:%02d:%02d,%s,%010d,%s:%s:%d,\t",
	           type,
	           tm->tm_year + 1900,
	           tm->tm_mon + 1,
	           tm->tm_mday,
	           tm->tm_hour,
	           tm->tm_min,
	           tm->tm_sec,
	           g_logLevToStrW[level],
	           g_logKey,
	           fileName,
	           funcName,
	           line
	          );
}

void LogLevel(const char * fileName,
              const char * funcName,
              int          line,
              const char * type,
              int          level,
              const char * fmt,
              ...)
{
	if (!g_LogOX) return;
	if (g_logLevel > level) return;

	char szLogBuff[1024];
	szLogBuff[0] = '\0';

	struct tm tm;
	GenLogHead(szLogBuff, &tm, fileName, funcName, type, line, level);
	int    szlogBufSz = strlen(szLogBuff);
	char * endBuf     = &szLogBuff[szlogBufSz];

	///LOG MSG////
	va_list ap;    // 가변 인자 목록 포인터
	va_start(ap,fmt);
	vsnprintf(endBuf, 1024 - szlogBufSz, fmt, ap);
	va_end(ap);
	///////////

	if (g_logOutOX)
		printf("%s\n", szLogBuff);
	char outFileName[100];
	sprintf_s(outFileName,
	          "%s/%s_%04d%02d.csv",
	          g_logDirName.c_str(),
	          type,
	          tm.tm_year + 1900,
	          tm.tm_mon + 1);

	// File Open Spin Wait
	FILE * file = nullptr;
	int    fret = fopen_s(&file, outFileName, "a+");
	while (fret != 0)
	{
		Yield();
		fret = fopen_s(&file, outFileName, "a+");
	}
	fprintf(file, "%s\n", szLogBuff);
	fclose(file);
}

void LogLevelW(const wchar_t * fileName,
               const wchar_t * funcName,
               int             line,
               const wchar_t * type,
               int             level,
               const wchar_t * fmt,
               ...)
{
	if (!g_LogOX) return;
	if (g_logLevel > level) return;

	wchar_t wszLogBuff[1024];
	wszLogBuff[0] = '\0';

	struct tm tm;
	GenLogHeadW(wszLogBuff, &tm, fileName, funcName, type, line, level);

	int       wszlogBufSz = wcslen(wszLogBuff);
	wchar_t * endBuf      = &wszLogBuff[wszlogBufSz];

	///LOG MSG////
	va_list ap;    // 가변 인자 목록 포인터
	va_start(ap, fmt);
	_vsnwprintf_s(endBuf, 1024 - wszlogBufSz, 1024 - wszlogBufSz, fmt, ap);
	va_end(ap);
	///////////

	if (g_logOutOX)
		wprintf(L"%s\n", wszLogBuff);

	SYSTEMTIME stNowT;
	char       tableName[100];

	//Get Table Name

	wchar_t outFileName[100];
	wsprintfW(outFileName,
	          L"%s/%s_%04d%02d.csv",
	          g_wlogDirName.c_str(),
			  type,
	          tm.tm_year + 1900,
	          tm.tm_mon + 1);

	// File Open Spin Wait
	FILE * file = nullptr;
	int    fret = _wfopen_s(&file, outFileName, L"a+");
	while (fret != 0)
	{
		Yield();
		fret = _wfopen_s(&file, outFileName, L"a+");
	}
	fwprintf(file, L"%s\n", wszLogBuff);
	fclose(file);
}

void LogHex(const char * fileName,
            const char * funcName,
            int          line,
            const char * type,
            int          level,
            char *       pbyte,
            int          bytelen)
{
	if (!g_LogOX) return;
	if (g_logLevel > level) return;

	char szLogBuff[1024];
	szLogBuff[0] = '\0';

	struct tm tm;
	GenLogHead(szLogBuff, &tm, fileName, funcName, type, line, level);

	// Append Byte
	int    szlogBufSz = strlen(szLogBuff);
	char * endBuf     = &szLogBuff[szlogBufSz];
	sprintf_s(endBuf, 3, "0x");
	endBuf += 2;


	int maxLen = (1024 - szlogBufSz) / 3;
	maxLen     = std::min(maxLen, bytelen);
	for (int i = 0; i < maxLen; ++i)
	{
		sprintf_s(endBuf, 4, "%x ", pbyte[i]);
		endBuf += 3;
	}

	////
	if (g_logOutOX)
		printf("%s\n", szLogBuff);
	char outFileName[100];
	sprintf_s(outFileName,
	          "%s/%s_%04d%02d.csv",
	          g_logDirName.c_str(),
	          type,
	          tm.tm_year + 1900,
	          tm.tm_mon + 1);

	// File Open Spin Wait
	FILE * file = nullptr;
	int    fret = fopen_s(&file, outFileName, "a+");
	while (fret != 0)
	{
		Yield();
		fret = fopen_s(&file, outFileName, "a+");
	}
	fprintf(file, "%s\n", szLogBuff);
	fclose(file);
}


void LogLevelProfile(const char * fileName,
                     const char * funcName,
                     int          line,
                     const char * type,
                     int          level,
                     char *       fmt,
                     ...)
{
	DWORD befTime = timeGetTime();

	if (!g_LogOX) return;
	if (g_logLevel > level) return;

	char szLogBuff[1024];
	szLogBuff[0] = '\0';

	struct tm tm;
	GenLogHead(szLogBuff, &tm, fileName, funcName, type, line, level);
	int    szlogBufSz = strlen(szLogBuff);
	char * endBuf     = &szLogBuff[szlogBufSz];

	///LOG MSG////
	va_list ap;    // 가변 인자 목록 포인터
	va_start(ap, fmt);
	vsnprintf(endBuf, 1024 - szlogBufSz, fmt, ap);
	va_end(ap);
	///////////

	if (g_logOutOX)
		printf("%s\n", szLogBuff);
	char outFileName[100];
	sprintf_s(outFileName,
	          "%s/%s_%04d%02d.csv",
	          g_logDirName.c_str(),
	          type,
	          tm.tm_year + 1900,
	          tm.tm_mon + 1);

	// File Open Spin Wait
	FILE * file = nullptr;
	int    fret = fopen_s(&file, outFileName, "a+");
	while (fret != 0)
	{
		Yield();
		fret = fopen_s(&file, outFileName, "a+");
	}
	fprintf(file, "%s\n", szLogBuff);
	fclose(file);

	g_logTimeInSec += timeGetTime() - befTime;
}
