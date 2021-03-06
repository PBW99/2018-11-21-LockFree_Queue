#include "SharedPCH.h"
#include <synchapi.h>

#define ENQUEUENUM 4
#define DATANUM ENQUEUENUM*10000000

#define DEQUEUENUM 4
bool              g_valid[DATANUM];
LF64::QueueT<int> g_TDQueue;
bool              g_worDone[ENQUEUENUM];
vector<int>       g_deqret[DEQUEUENUM];

SRWLOCK lock = SRWLOCK_INIT;


unsigned _stdcall TestThread(void * param)
{
	int      tid   = *(int*)&param;
	int      chunk = DATANUM / ENQUEUENUM;
	for (int i     = tid * chunk; i < (tid + 1) * chunk; ++i)
	{
		g_TDQueue.Enqueue(i);
	}
	g_worDone[tid] = true;
	return 0;
}

unsigned _stdcall Reducer(void * param)
{
	int tid = *(int*)&param;
	g_deqret[tid].clear();
	while (1)
	{
		bool     alldone = true;
		for (int i       = 0; i < ENQUEUENUM; ++i)
		{
			if (!g_worDone[i])
			{
				alldone = false;
			}
		}
		if (alldone)break;

		int data;
		int ret = g_TDQueue.Dequeue(data);
		if (ret == 0)
		{
			g_valid[data] = true;
			g_deqret[tid].emplace_back(data);
		}
	}
	int ret = 0;
	while (ret == 0)
	{
		int data;
		ret = g_TDQueue.Dequeue(data);
		if (ret == 0)
		{
			g_valid[data] = true;
			g_deqret[tid].emplace_back(data);
		}
	}


	return 0;
}


int main()
{
	timeBeginPeriod(0);
	while (1)
	{
		HANDLE thds[ENQUEUENUM];
		HANDLE thdreduce[DEQUEUENUM];

		for (int i = 0; i < DATANUM; ++i)
		{
			g_valid[i] = false;
		}
		for (int i = 0; i < ENQUEUENUM; ++i)
		{
			g_worDone[i] = false;
		}

		DWORD    starttime = timeGetTime();
		for (int i         = 0; i < ENQUEUENUM; ++i)
		{
			thds[i] = (HANDLE)_beginthreadex(0, 0, TestThread, (void*)i, 0, 0);
		}
		for (int i = 0; i < DEQUEUENUM; ++i)
		{
			thdreduce[i] = (HANDLE)
				_beginthreadex(0, 0, Reducer, (void*)i, 0, 0);
		}

		while (1)
		{
			DWORD waitret = WaitForMultipleObjects(ENQUEUENUM,
			                                       thds,
			                                       true,
			                                       1000);
			if (waitret != WAIT_TIMEOUT) break;
			cout << "___Q Size : " << g_TDQueue.GetSize() << "\n";
		}
		WaitForMultipleObjects(DEQUEUENUM, thdreduce, true, INFINITE);
		DWORD endtime = timeGetTime();

		bool        valid = true;
		vector<int> invalids;
		for (int    i = 0; i < DATANUM; ++i)
		{
			if (!g_valid[i])
			{
				valid = false;
				invalids.push_back(i);
			}
		}

		cout << "[Done Size : " << g_TDQueue.GetSize()
			<< "Perf Time : " << endtime - starttime
			<< ", Valid : " << valid
			<< "]\n";
		cout << " >> Invalids : ";
		for (auto invalid : invalids)
		{
			cout << invalid << ", ";
		}
		cout << "\n";
		if (!valid)
		{
			UtilCrashDump::Crash();
		}
	}
}
