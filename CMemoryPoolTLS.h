#pragma once

// #define DEBUG_MEM_TLS
#ifdef DEBUG_MEM_TLS
#define OND_MEM_TLS(...) __VA_ARGS__;
#else
#define OND_MEM_TLS(...);
#endif
namespace LF64
{
	template <typename T>
	class CMemoryPoolTLS
	{
#define dfTCHUNK_SIZE 200

		class TChunk;
		// Child Type
		typedef struct TWrap
		{
			TChunk * pChunk;
			T        t;
		}            TWrap;

		class TChunk
		{
			friend class CMemoryPoolTLS;
		public:
			TChunk()
			{
				for (int i = 0; i < dfTCHUNK_SIZE; ++i)
				{
					new(&data[i].t) T();
				}
			}

			void Clear(int ownerID)
			{
				if (!IsAllAllocated())
				{
					LOG("Engine", dfLOG_LEVEL_ERROR, "Wrong Chunk");
					UtilCrashDump::Crash();
				}
				if (!IsAllFreed())
				{
					LOG("Engine", dfLOG_LEVEL_ERROR, "Wrong Chunk");
					UtilCrashDump::Crash();
				}

				for (int i = 0; i < dfTCHUNK_SIZE; ++i)
				{
					data[i].pChunk = this;
				}
				ownerId = ownerID;
				top = dfTCHUNK_SIZE;
				freeCount = dfTCHUNK_SIZE;
				OND_MEM_TLS(
					q.push(10 * ownerID);
				if (q.size() > 100) q.pop();
				)
			}

			// If All chunk allocated, return nullptr
			T* Alloc()
			{
				if (IsAllAllocated())
				{
					LOG("Engine", dfLOG_LEVEL_ERROR, "No more data");
					UtilCrashDump::Crash();
				}
				T * ret = &data[top - 1].t;
				top--;
				alloced++;
				return ret;
			}

			bool Free(T * p)
			{
				int lfreec = freeCount;
				int ltop = top;
				if (ltop > lfreec)
				{
					LOG("Engine", dfLOG_LEVEL_ERROR,
						"More Free than Alloc! top %d, freecount %d", ltop,
						lfreec);
					UtilCrashDump::Crash();
				}

				int retFC = InterlockedDecrement((LONG*)&freeCount);
				InterlockedIncrement((LONG*)&freed);
				if (retFC == 0)return false;

				return true;
			}

			bool IsAllAllocated()
			{
				return top == 0;
			}

			bool IsAllFreed()
			{
				return freeCount == 0;
			}

		private:
			TWrap data[dfTCHUNK_SIZE];
			int   ownerId = -1;
			int   top = 0;
			int   freeCount = 0;
			int   alloced = 0;
			int   freed = 0;
			OND_MEM_TLS(
				queue<int> q;
			)
		};

	public:
		CMemoryPoolTLS(int  initPoolSize,
			bool isPlacementNew,
			int  threadNum
		) :
			mInitPoolsize(initPoolSize),
			isPlacementNew(isPlacementNew),
			mThreadNum(threadNum)
		{
			mPool = new CMemoryPool<TChunk>(mInitPoolsize / dfTCHUNK_SIZE,
				isPlacementNew);
			mChunks = new TChunk*[mThreadNum];
			memset(mChunks, 0, sizeof(TChunk*) * mThreadNum);
			mTlsIdx = TlsAlloc();

			OND_MEM_TLS(
				mChunkGets = new int[mThreadNum];
			mChunkPuts = new int[mThreadNum];
			memset(mChunkGets, 0, sizeof(int) * mThreadNum);
			memset(mChunkPuts, 0, sizeof(int) * mThreadNum);
			)
		}

		virtual ~CMemoryPoolTLS()
		{
			// TODO
			//Free All Chunks
			delete mPool;
			delete[] mChunks;
		}


		T* Alloc()
		{
			void * val = TlsGetValue(mTlsIdx);
			if (!val)
			{
				AcquireSRWLockExclusive(&mChunkLock);
				int64_t i = 0;
				for (i = mChunkSeed; i < mThreadNum; ++i)
				{
					if (mChunks[i] == nullptr)
					{
						TlsSetValue(mTlsIdx, (void*)(i + 1));
						val = (void*)(i + 1);
						mChunks[i] = GetChunk();
						mChunkSeed = i + 1;
						break;
					}
				}
				ReleaseSRWLockExclusive(&mChunkLock);
				if (i == mThreadNum)
				{
					LOG("Engine", dfLOG_LEVEL_ERROR, "Over ThreadNum");
					UtilCrashDump::Crash();
				}
			}
			int      intval = (int)val - 1;
			TChunk * target = mChunks[intval];
			if (target == nullptr)
			{
				mChunks[intval] = GetChunk();
				target = mChunks[intval];
			}
			T * ret = target->Alloc();
			if (target->IsAllAllocated())
			{
				mChunks[intval] = nullptr;
			}
			InterlockedIncrement((LONG*)&mUseCount);


			return ret;
		}

		void Free(T * p)
		{
			TWrap * twrap = (TWrap*)((char*)p - sizeof(TChunk*));
			bool    ret = twrap->pChunk->Free(p);
			if (!ret)
			{
				CleanChunk(twrap->pChunk);
			}
			InterlockedDecrement((LONG*)&mUseCount);
		}

		int GetCreateCount() const
		{
			return mPool->GetCreateCount() * dfTCHUNK_SIZE;
		}

		int GetUseCount() const
		{
			return mUseCount;
		};


		bool CheckThreadGetPutEqual()
		{
			bool ret = true;
			OND_MEM_TLS(
				void * val = TlsGetValue(mTlsIdx);
			int intval = (int)val - 1;
			ret = mChunkGets[intval] == mChunkPuts[intval];

			if (!ret)
			{
				UtilCrashDump::Crash();
			}
			)

				return ret;
		}

		bool CheckThreadChunkIsFull()
		{
			bool ret = true;
			OND_MEM_TLS(
				void * val = TlsGetValue(mTlsIdx);
			int intval = (int)val - 1;
			if (!val)return true;
			if (!mChunks[intval])return true;
			ret = mChunks[intval]->IsAllAllocated();

			if (!ret)
			{
				UtilCrashDump::Crash();
			}
			)
				return ret;
		}

		bool CheckThreadChunkIsFreed()
		{
			bool ret = true;
			OND_MEM_TLS(
				void * val = TlsGetValue(mTlsIdx);
			int intval = (int)val - 1;
			ret = mChunks[intval] == nullptr;

			if (!ret)
			{
				UtilCrashDump::Crash();
			}
			)
				return ret;
		}


		friend std::ostream& operator<<(std::ostream &         os,
			const CMemoryPoolTLS & obj)
		{
			return os
				<< " mCreateCount: " << obj.GetCreateCount()
				<< "\n mUseCount: " << obj.GetUseCount()
				<< "\n";
		}

	private:

		void CleanChunk(TChunk * pChunk)
		{
			OND_MEM_TLS(
				if (!pChunk->IsAllAllocated())
				{
					LOG("Engine", dfLOG_LEVEL_ERROR, "Wrong Chunk");
					UtilCrashDump::Crash();
				}
			if (!pChunk->IsAllFreed())
			{
				LOG("Engine", dfLOG_LEVEL_ERROR, "Wrong Chunk");
				UtilCrashDump::Crash();
			}
			pChunk->q.push(pChunk->ownerId);
			if (pChunk->q.size() > 100) pChunk->q.pop();

			mChunkPuts[pChunk->ownerId]++;
			InterlockedIncrement((LONG*)&mChunkFreeCount);
			)
				mPool->Free(pChunk);
		}

		TChunk* GetChunk()
		{
			void * val = TlsGetValue(mTlsIdx);
			int    intval = (int)val - 1;

			TChunk * ret;
			ret = mPool->Alloc();
			ret->Clear(intval);

			OND_MEM_TLS(
				mChunkGets[intval]++;
			InterlockedIncrement((LONG*)&mChunkGetCount);
			)

				return ret;
		}

		CMemoryPool<TChunk> * mPool;

		int  mInitPoolsize;
		bool isPlacementNew;

		int mTlsIdx;
		int mThreadNum;

		SRWLOCK   mChunkLock = SRWLOCK_INIT;
		TChunk ** mChunks;
		int       mChunkSeed = 0;
		int       mUseCount = 0;
		OND_MEM_TLS(
			int * mChunkGets;
		int * mChunkPuts;
		int mChunkGetCount = 0;
		int mChunkFreeCount = 0;
		)
	};
}
