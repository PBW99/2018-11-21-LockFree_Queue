#include "SharedPCH.h"

#define dfTHREAD_NUM 8
#define dfTEST_CHUNK_SIZE 2
#define dfTEST_SLEEP_TIME 0


typedef struct TestData
{
	LONG64 data  = 0;
	LONG   count = 0;
}          TestData;

UtilCrashDump *        g_crashDump = UtilCrashDump::GetIns();
LF64::QueueT<TestData> g_TDQueue;
bool                   g_err = false;

unsigned _stdcall TestThread(void * param)
{
	TestData tdarr[dfTEST_CHUNK_SIZE];
	while (1)
	{
		if (g_err) break;
		// 0. Alloc (스레드당 10000 개 x 10 개 스레드 총 10만개)
		for (int i = 0; i < dfTEST_CHUNK_SIZE; ++i)
		{
			int ret = -1;
			while (ret != 0)
			{
				ret = g_TDQueue.Dequeue(tdarr[i]);
			}
		}
		// 1. 0x0000000055555555 이 맞는지 확인.
		// 2. Interlocked + 1 (Data + 1 / Count + 1)
		for (int i = 0; i < dfTEST_CHUNK_SIZE; ++i)
		{
			if (tdarr[i].data != 0x0000000055555555)
			{
				printf("data isn't 0x0000000055555555!!!!\n");
				g_err = true;
				UtilCrashDump::Crash();
				break;
			}
			InterlockedIncrement64((LONG64*)&tdarr[i].data);
			InterlockedIncrement((LONG*)&tdarr[i].count);
		}
		// 3. 약간대기
		Yield();

		// 4. 여전히 0x0000000055555556 이 맞는지 (Count == 1) 확인.
		// 5. Interlocked - 1 (Data - 1 / Count - 1)
		for (int i = 0; i < dfTEST_CHUNK_SIZE; ++i)
		{
			if (tdarr[i].data != 0x0000000055555556)
			{
				printf("data isn't 0x0000000055555556!!!!\n");
				g_err = true;
				UtilCrashDump::Crash();
				break;
			}
			if (tdarr[i].count != 1)
			{
				printf("count isn't 1!!!!\n");
				g_err = true;
				UtilCrashDump::Crash();
				break;
			}

			InterlockedDecrement64((LONG64*)&tdarr[i].data);
			InterlockedDecrement((LONG*)&tdarr[i].count);
		}
		// 6. 약간대기
		Yield();

		// 7. 0x0000000055555555 이 맞는지 (Count == 0) 확인.
		// 8. Free
		for (int i = 0; i < dfTEST_CHUNK_SIZE; ++i)
		{
			if (tdarr[i].data != 0x0000000055555555)
			{
				printf("data isn't 0x0000000055555555!!!!\n");
				g_err = true;
				UtilCrashDump::Crash();
				break;
			}
			if (tdarr[i].count != 0)
			{
				printf("count isn't 0!!!!\n");
				g_err = true;
				UtilCrashDump::Crash();
				break;
			}
			g_TDQueue.Enqueue(tdarr[i]);
		}
		// 반복.
	}

	return 0;
}

int main()
{
	int a;
	int b;
	int *pa =&a;
	int *pb =&b;
	a=1;
	b=2;

	HANDLE thds[dfTHREAD_NUM];

	for (int i = 0; i < dfTHREAD_NUM * dfTEST_CHUNK_SIZE; ++i)
	{
		TestData td;
		td.data = 0x0000000055555555;
		g_TDQueue.Enqueue(td);
	}
	cout << "Size: " << g_TDQueue.GetSize()<< "\n";


	for (int i = 0; i < dfTHREAD_NUM; ++i)
	{
		thds[i] = (HANDLE)_beginthreadex(0, 0, TestThread, (void*)i, 0, 0);
	}
	while (1)
	{
		Sleep(1000);
		if (g_err) break;
		printf("Size: %d \n",
		       g_TDQueue.GetSize());
	}
	WaitForMultipleObjects(dfTHREAD_NUM, thds, true, INFINITE);

	cout << "Hello World!\n";
}
