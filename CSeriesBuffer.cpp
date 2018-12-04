#include "SharedPCH.h"

LF64::CMemoryPoolTLS<CSeriesBuffer> * CSeriesBuffer::sCsbPool;

void CSeriesBuffer::IncreRef()
{
	InterlockedIncrement((long*)&mRefCnt);
}

int CSeriesBuffer::GetRefCnt()
{
	return mRefCnt;
}

CSeriesBuffer::CSeriesBuffer():
	mBufferSize(UtilDef::SERIES_BUFFER_DEFAULT_SIZE)
{
	mBuffer = new char[mBufferSize];
}

CSeriesBuffer::CSeriesBuffer(int bufferSize, int payloadStart, int headSize):
	mFront(payloadStart),
	mRear(payloadStart),
	mBufferSize(bufferSize),
	mPayloadStart(payloadStart),
	mHeadSize(headSize),
	mHeadStart(payloadStart - headSize)
{
	mBuffer = new char[mBufferSize];
}

// CSeriesBuffer::CSeriesBuffer(const CSeriesBuffer & rhs)
// {
// 	CopyFrom(rhs);
// 	// cout << "COPY FROM()\n";
// }

void CSeriesBuffer::Setup(int threadNum)
{
	sCsbPool = new LF64::CMemoryPoolTLS<CSeriesBuffer>(0, false, threadNum);
}

CSeriesBuffer::~CSeriesBuffer()
{
	Release();
}

void CSeriesBuffer::Release()
{
	if (mBuffer != nullptr)
		delete[] mBuffer;
}

void CSeriesBuffer::Clear()
{
	mFront = mPayloadStart;
	mRear  = mPayloadStart;
}

int CSeriesBuffer::GetBufferSize()
{
	return mBufferSize;
}

int CSeriesBuffer::GetDataSize() const
{
	return mRear - mFront;
}

char* CSeriesBuffer::GetBufferPtr()
{
	return &mBuffer[mPayloadStart];
}


int CSeriesBuffer::MoveWritePos(int iSize)
{
	if (iSize < 0)
	{
		LOGW(L"RINGBF",dfLOG_LEVEL_DEBUG,L"Wrong Size in MoveWritePos()");
		return -1;
	}
	if (mRear + iSize > mBufferSize)
	{
		LOGW(L"RINGBF", dfLOG_LEVEL_DEBUG,
			L"CSeriesBuffer Overflow in MoveWritePos()");
		return -1;
	}
	mRear = mRear + iSize;
	return 0;
}

int CSeriesBuffer::MoveReadPos(int iSize)
{
	if (iSize < 0)
	{
		LOGW(L"RINGBF", dfLOG_LEVEL_DEBUG,L"Wrong Size in MoveWritePos()");
		return -1;
	}
	if (mFront + iSize > mBufferSize)
	{
		LOGW(L"RINGBF", dfLOG_LEVEL_DEBUG,
			L"CSeriesBuffer Overflow in MoveReadPos()");
		return -1;
	}
	mFront = mFront + iSize;
	return 0;
}

CSeriesBuffer& CSeriesBuffer::operator=(CSeriesBuffer & clSrcPacket)
{
	if (this == &clSrcPacket)
	{
		return *this;
	}
	Release();
	CopyFrom(clSrcPacket);
	return *this;
}
//
int CSeriesBuffer::GetData(char * chpDest, int iSize)
{

	if (mFront + iSize > mRear)
	{
		LOGW(L"RINGBF", dfLOG_LEVEL_DEBUG,L"CSeriesBuffer Overflow in GetData()"
		);
		throw CSeriesBufferException();
		return -1;
	}

	memcpy(chpDest, &mBuffer[mFront], iSize);
	MoveReadPos(iSize);

	return 0;
}

int CSeriesBuffer::PutData(char * chpSrc, int iSrcSize)
{

	if (mRear + iSrcSize > mBufferSize)
	{
		LOGW(L"RINGBF", dfLOG_LEVEL_DEBUG,L"CSeriesBuffer Overflow in PutData()"
		);
		throw CSeriesBufferException();
		return -1;
	}

	memcpy(&mBuffer[mRear], chpSrc, iSrcSize);
	MoveWritePos(iSrcSize);

	return 0;
}

char* CSeriesBuffer::GetHeadBufferPtr()
{
	return &mBuffer[mHeadStart];
}

int CSeriesBuffer::GetAllDataSize() const
{
	return mRear - mHeadStart;
}

void CSeriesBuffer::GetHead(char * outhead)
{
	memcpy(outhead, &mBuffer[mHeadStart], mHeadSize);
}

void CSeriesBuffer::PutHead(char * inhead)
{
	memcpy(&mBuffer[mHeadStart], inhead, mHeadSize);
}

CSeriesBuffer* CSeriesBuffer::Alloc()
{
	CSeriesBuffer * ret = sCsbPool->Alloc();
	ret->Clear();
	ret->mRefCnt = 1;

	return ret;
}

void CSeriesBuffer::Free(CSeriesBuffer * p)
{
	int ret = InterlockedDecrement((long*)&p->mRefCnt);
	if (ret == 0)
	{
		sCsbPool->Free(p);
	}
}

void CSeriesBuffer::CopyFrom(const CSeriesBuffer & rhs)
{
	mFront        = rhs.mFront;
	mRear         = rhs.mRear;
	mBufferSize   = rhs.mBufferSize;
	mPayloadStart = rhs.mPayloadStart;
	mHeadSize     = rhs.mHeadSize;
	mHeadStart    = rhs.mHeadStart;
	mBuffer       = new char[rhs.mBufferSize];
	memcpy_s(mBuffer, mBufferSize, rhs.mBuffer, rhs.mBufferSize);
}
