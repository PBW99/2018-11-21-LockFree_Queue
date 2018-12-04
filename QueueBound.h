#pragma once

namespace LF
{
	template <typename T>
	class QueueBound
	{
	public:
		QueueBound(const LONG & size)
			: mSize(size)
		{
			mBuf   = new Node[mSize];
			mFront = -1;
			mRear  = -1;
		}

		virtual ~QueueBound()
		{
			delete[] mBuf;
		}

		int enqueue(T t)
		{
			LONG curRear  = InterlockedIncrement(&mRear);
			LONG curRound = curRear / mSize;
			LONG curPos   = curRear % mSize;

			while (1)
			{
				LONG targetFlag  = mBuf[curPos].flag;
				LONG targetRound = targetFlag / 2;
				LONG targetIsOdd = targetFlag % 2;
				if (targetRound == curRound && !targetIsOdd)
				{
					break;
				}
				Yield();
			}
			mBuf[curPos].t = t;
			InterlockedIncrement(&mBuf[curPos].flag);
			return 0;
		}

		T dequeue()
		{
			LONG curFront = InterlockedIncrement(&mFront);
			LONG curRound = curFront / mSize;
			LONG curPos   = curFront % mSize;

			while (1)
			{
				LONG targetFlag  = mBuf[curPos].flag;
				LONG targetRound = targetFlag / 2;
				LONG targetIsOdd = targetFlag % 2;
				if (targetRound == curRound && targetIsOdd)
				{
					break;
				}
				Yield();
			}
			T ret = mBuf[curPos].t;
			InterlockedIncrement(&mBuf[curPos].flag);

			return ret;
		}
		int GetUseSize()
		{
			return mRear - mFront;
		}

	private:
		struct Node
		{
			LONG flag = 0;
			T    t;
		};

		Node * mBuf;
		LONG   mFront;
		LONG   mRear;
		LONG   mSize;
	};
}
