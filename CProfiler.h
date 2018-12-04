#pragma once

#pragma comment(lib,"kernel32")

#define dfPROFILE_FUNCNAME_MAX 50

typedef struct CProfiler
{
	void   InitCProfiler(const char * tagName);;
	void   Begin();
	bool   End();
	double GetAverageInMicro();
	double GetMinInMicro();
	double GetMaxInMicro();

	bool     isValid = false;
	bool     isBegin = false;
	char     mTagName[dfPROFILE_FUNCNAME_MAX+1];
	uint64_t callNum = 0;
	uint64_t min[2]; // min[0] : 2nd Min, min[1] : 1st min
	uint64_t max[2]; // max[0] : 2nd Min, max[1] : 1st max
	uint64_t start = 0;
	uint64_t total = 0;
}            CProfiler;

typedef struct ReducCProfiler
{
	void   InitCProfiler(const char * tagName);;
	void ProcMin(uint64_t diff);
	void ProcMax(uint64_t diff);
	void ProcTotal(uint64_t argTotal,uint64_t argCallNum);
	double GetAverageInMicro();
	double GetMinInMicro();
	double GetMaxInMicro();

	bool     isValid = false;
	bool     isBegin = false;
	char     mTagName[dfPROFILE_FUNCNAME_MAX+1];
	uint64_t callNum = 0;
	uint64_t min[2]; // min[0] : 2nd Min, min[1] : 1st min
	uint64_t max[2]; // max[0] : 2nd Min, max[1] : 1st max
	uint64_t start = 0;
	uint64_t total = 0;
}            ReducCProfiler;