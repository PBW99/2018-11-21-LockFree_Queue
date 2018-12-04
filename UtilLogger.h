#pragma once

//Global variables
extern bool   g_LogOX;
extern bool   g_logOutOX;
extern int    g_logLevel;
extern string g_logDirName;
extern wstring g_wlogDirName;

extern DWORD                           g_logTimeInSec;
extern unordered_map<int, const char*> g_logLevToStr;
extern unordered_map<int, const wchar_t*> g_logLevToStrW;
extern DWORD                           g_logKey;

void SetupMacroLogger(bool         outOX,
                      int          logLevel,
                      const char * logDirName
);
void ShowLogStatus();


//Runtime Level Check Logger
#define dfLOG_LEVEL_DEBUG 0
#define dfLOG_LEVEL_WARNING 1
#define dfLOG_LEVEL_ERROR 2
#define dfLOG_LEVEL_SYSTEM 3

#define __FILENAME__(...) (strrchr(__VA_ARGS__, '\\') ? strrchr(__VA_ARGS__, '\\') + 1 : __VA_ARGS__)
#define __FILENAMEW__(...) (wcsrchr(__VA_ARGS__, '\\') ? wcsrchr(__VA_ARGS__, '\\') + 1 : __VA_ARGS__)
#define LOG(...) LogLevel(__FILENAME__(__FILE__),__FUNCTION__, __LINE__,__VA_ARGS__)
#define LOGW(...) LogLevelW(__FILENAMEW__(__FILEW__),__FUNCTIONW__, __LINE__,__VA_ARGS__)
#define LOG_HEX(...) LogHex(__FILENAME__(__FILE__),__FUNCTION__, __LINE__,__VA_ARGS__)
#define LOG_PF(...) LogLevelProfile(__FILENAME__(__FILE__),__FUNCTION__, __LINE__,__VA_ARGS__)

void GenLogHead(OUT char *           buf,
                    OUT struct tm *  tm,
                    const char *     fileName,
                    const char *     funcName,
                    const char *     type,
                    int              line,
                    int              level);

void GenLogHeadW(OUT wchar_t *       buf,
                     OUT struct tm * tm,
                     const wchar_t * fileName,
                     const wchar_t * funcName,
                     const wchar_t * type,
                     int             line,
                     int             level);

void LogLevel(const char * fileName,
              const char * funcName,
              int          line,
              const char * type,
              int          level,
              const char * fmt,
              ...);

void LogLevelW(const wchar_t * fileName,
               const wchar_t * funcName,
               int             line,
               const wchar_t * type,
               int             level,
               const wchar_t * fmt,
               ...);
void LogHex(const char * fileName,
	const char * funcName,
	int          line,
	const char * type,
	int          level,
	char * pbyte,
	int bytelen);

void LogLevelProfile(const char * fileName,
                     const char * funcName,
                     int          line,
                     const char * type,
                     int          level,
                     char *       fmt,
                     ...);
