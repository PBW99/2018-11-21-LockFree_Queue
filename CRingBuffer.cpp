#include "SharedPCH.h"

int CRingBuffer::Enqueue(char * buffer, int size)
{
	// Get Parameter Overflow
	int rear          = mRear;
	int freeSize      = GetFreeSize();
	int overflowSize  = size - freeSize > 0 ? size - freeSize : 0;
	int availableSize = size - overflowSize;

	// Get Block Overflow
	int blockOverflowSize = (availableSize + rear) - mQueueSize > 0
		                        ? (availableSize + rear) - mQueueSize
		                        : 0;
	int firstBlockSize  = availableSize - blockOverflowSize;
	int secondBlockSize = availableSize - firstBlockSize;

	// Copy Each Block
	memcpy(&mBuffer[rear], buffer, firstBlockSize);
	if (secondBlockSize > 0)
		memcpy(mBuffer, buffer + firstBlockSize, secondBlockSize);

	// Move Rear
	mRear = (rear + availableSize) % mQueueSize;

	return availableSize;
}

int CRingBuffer::Dequeue(char * buffer, int size)
{
	int front         = mFront;
	int useSize       = GetUseSize();
	int overflowSize  = size - useSize > 0 ? size - useSize : 0;
	int availableSize = size - overflowSize;

	int blockOverflowSize = (availableSize + front) - mQueueSize > 0
		                        ? (availableSize + front) - mQueueSize
		                        : 0;
	int firstBlockSize  = availableSize - blockOverflowSize;
	int secondBlockSize = availableSize - firstBlockSize;

	memcpy(buffer, &mBuffer[front], firstBlockSize);
	if (secondBlockSize > 0)
		memcpy(buffer + firstBlockSize, mBuffer, secondBlockSize);

	MemoryBarrier();

	mFront = (front + availableSize) % mQueueSize;

	return availableSize;
}

int CRingBuffer::Peek(char * buffer, int size)
{
	int front         = mFront;
	int useSize       = GetUseSize();
	int overflowSize  = size - useSize > 0 ? size - useSize : 0;
	int availableSize = size - overflowSize;

	int blockOverflowSize = (availableSize + front) - mQueueSize > 0
		                        ? (availableSize + front) - mQueueSize
		                        : 0;
	int firstBlockSize  = availableSize - blockOverflowSize;
	int secondBlockSize = availableSize - firstBlockSize;

	memcpy(buffer, &mBuffer[front], firstBlockSize);
	memcpy(buffer + firstBlockSize, mBuffer, secondBlockSize);

	return firstBlockSize + secondBlockSize;
}

int CRingBuffer::GetUseSize() const
{
	int ret;
	int front = mFront;
	int rear  = mRear;
	if (rear == front)
	{
		ret = 0;
	}
	else if (rear > front)
	{
		ret = rear - front;
	}
	else
	{
		ret = mQueueSize - front + rear;
	}
	if (ret >= mQueueSize - 1)
	{
		LOGW(L"Engine",dfLOG_LEVEL_ERROR,L"RingBuffer Full!");
	}
	return ret;
}

int CRingBuffer::GetFreeSize() const
{
	return mQueueSize - GetUseSize() - CRingBuffer::DEF::EMPTY_BYTE;
}

int CRingBuffer::GetNotBrokenGetSize()
{
	int availableSize = GetUseSize();

	int blockOverflowSize = (availableSize + mFront) - mQueueSize > 0
		                        ? (availableSize + mFront) - mQueueSize
		                        : 0;
	return availableSize - blockOverflowSize;
}

int CRingBuffer::GetNotBrokenPutSize()
{
	int availableSize = GetFreeSize();

	int blockOverflowSize = (availableSize + mRear) - mQueueSize > 0
		                        ? (availableSize + mRear) - mQueueSize
		                        : 0;
	return availableSize - blockOverflowSize;
}

void CRingBuffer::MoveRear(int iSize)
{
	mRear = (mRear + iSize) % mQueueSize;
}

void CRingBuffer::MoveFront(int iSize)
{
	mFront = (mFront + iSize) % mQueueSize;
}

void CRingBuffer::ClearBuffer()
{
	mFront = 0;
	mRear  = 0;
}

char* CRingBuffer::GetBufferPtr()
{
	return mBuffer;
}

char* CRingBuffer::GetFrontBufferPtr()
{
	return &mBuffer[mFront];
}

char* CRingBuffer::GetRearBufferPtr()
{
	return &mBuffer[mRear];
}

int CRingBuffer::GetFront()
{
	return mFront;
}

int CRingBuffer::GetRear()
{
	return mRear;
}
