#include "SharedPCH.h"


void CProfiler::InitCProfiler(const char * tagName)
{
	strncpy_s(mTagName, tagName,dfPROFILE_FUNCNAME_MAX);
	isValid = true;
	min[0]  = min[1] = ULLONG_MAX;
	max[0]  = max[1] = 0;
}

void CProfiler::Begin()
{
	LARGE_INTEGER startCnt;

	QueryPerformanceCounter(&startCnt);
	start   = startCnt.QuadPart;
	isBegin = true;
}

bool CProfiler::End()
{
	LARGE_INTEGER endCnt;
	uint64_t      end;
	uint64_t      diff;

	if (!isBegin)
	{
		return false;
	}

	QueryPerformanceCounter(&endCnt);
	end  = endCnt.QuadPart;
	diff = end - start;

	if (diff < min[0])
	{
		if (diff < min[1])
		{
			min[0] = min[1];
			min[1] = diff;
		}
		else
		{
			min[0] = diff;
		}
	}
	else if (diff > max[0])
	{
		if (diff > max[1])
		{
			max[0] = max[1];
			max[1] = diff;
		}
		else
		{
			max[0] = diff;
		}
	}

	total += diff;
	callNum++;
	isBegin = false;

	return true;
}


double CProfiler::GetAverageInMicro()
{
	if (callNum <= 2) return -1;
	uint64_t totalCutted   = total - min[1] - max[1];
	uint64_t callNumCutted = callNum - 2;

	return (double)totalCutted / (double)callNumCutted / (double)
		CProfilerManager::sfreq * (double)
		1000000;
}

double CProfiler::GetMinInMicro()
{
	if (callNum <= 2) return -1;
	return (double)min[0] / (double)CProfilerManager::sfreq * (double)1000000;
}

double CProfiler::GetMaxInMicro()
{
	if (callNum <= 2) return -1;
	return (double)max[0] / (double)CProfilerManager::sfreq * (double)1000000;
}

void ReducCProfiler::InitCProfiler(const char * tagName)
{
	strncpy_s(mTagName, tagName,dfPROFILE_FUNCNAME_MAX);
	isValid = true;
	min[0]  = min[1] = ULLONG_MAX;
	max[0]  = max[1] = 0;
}

void ReducCProfiler::ProcMin(uint64_t diff)
{
	if (diff < min[0])
	{
		if (diff < min[1])
		{
			min[0] = min[1];
			min[1] = diff;
		}
		else
		{
			min[0] = diff;
		}
	}
}

void ReducCProfiler::ProcMax(uint64_t diff)
{
	if (diff > max[0])
	{
		if (diff > max[1])
		{
			max[0] = max[1];
			max[1] = diff;
		}
		else
		{
			max[0] = diff;
		}
	}
}

void ReducCProfiler::ProcTotal(uint64_t argTotal, uint64_t argCallNum)
{
	total += argTotal;
	callNum += argCallNum;
}

double ReducCProfiler::GetAverageInMicro()
{
	uint64_t totalCutted   = total - min[1] - max[1];
	uint64_t callNumCutted = callNum - 2;

	return (double)totalCutted / (double)callNumCutted / (double)
		CProfilerManager::sfreq * (double)
		1000000;
}

double ReducCProfiler::GetMinInMicro()
{
	return (double)min[1] / (double)CProfilerManager::sfreq * (double)1000000;
}

double ReducCProfiler::GetMaxInMicro()
{
	return (double)max[1] / (double)CProfilerManager::sfreq * (double)1000000;
}
