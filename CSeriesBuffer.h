#pragma once

#define dfSERIES_BUF_DEFAULT_PAYLOAD_START 4
#define dfSERIES_BUF_DEFAULT_HEAD_SIZE 2
#define dfSERIES_BUF_DEFAULT_HEAD_START dfSERIES_BUF_DEFAULT_PAYLOAD_START-dfSERIES_BUF_DEFAULT_HEAD_SIZE


//////////////////////////////////////////
// maxhead : 5 headsize : 2
//|0|1|2|3|4|5|6|7| | | | | | |
//      ^   ^MaxHead|PayloadStart (rear, front)
//      ^HeadStart()
//////////////////////////////////////////
/*
 *
 *	!.	삽입되는 데이타 FIFO 순서로 관리된다.
		큐가 아니므로, 넣기(<<).빼기(>>) 를 혼합해서 사용하면 안된다.
		
 */
class CSeriesBuffer
{
	friend class LF64::CMemoryPool<CSeriesBuffer>;
	friend class LF64::CMemoryPoolTLS<CSeriesBuffer>;
public:
	CSeriesBuffer(const CSeriesBuffer & rhs) = delete;
	static void Setup(int threadNum);

	virtual ~CSeriesBuffer();
	void    Release();
	void    Clear();


	int   GetBufferSize(void);
	int   GetDataSize(void  ) const;
	char* GetBufferPtr(void );

	int MoveWritePos(int iSize);
	int MoveReadPos(int  iSize);


	CSeriesBuffer& operator =(CSeriesBuffer & clSrcPacket);


	template <typename T>
	CSeriesBuffer& operator >>(T & value)
	{
		static_assert( std::is_arithmetic<T>::value ||
			std::is_enum<T>::value ,
			"Generic Read only supports primitive data types" );
		GetData((char*)&value, sizeof(T));
		return *this;
	};

	template <typename T>
	CSeriesBuffer& operator <<(T & value)
	{
		static_assert( std::is_arithmetic<T>::value ||
			std::is_enum<T>::value,
			"Generic Read only supports primitive data types" );
		PutData((char*)&value, sizeof(T));
		return *this;
	};

	//////////////////////////////////////////////////////////////////////////
	// 데이타 얻기.
	//
	// Parameters: (char *)Dest 포인터. (int)Size.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int GetData(char * chpDest, int iSize);

	//////////////////////////////////////////////////////////////////////////
	// 데이타 삽입.
	//
	// Parameters: (char *)Src 포인터. (int)SrcSize.
	// Return: (int)복사한 사이즈.
	//////////////////////////////////////////////////////////////////////////
	int PutData(char * chpSrc, int iSrcSize);

	char* GetHeadBufferPtr(void);
	int   GetAllDataSize(void  ) const;
	void  GetHead(char *       outhead);
	void  PutHead(char *       inhead);

	static CSeriesBuffer* Alloc();
	static void Free(CSeriesBuffer* p);
	static LF64::CMemoryPoolTLS<CSeriesBuffer>*  sCsbPool;

	void IncreRef();
	int GetRefCnt();

private:
	CSeriesBuffer();
	CSeriesBuffer(int                   bufferSize,
	              int                   payloadStart,
	              int                   headSize);
	void CopyFrom(const CSeriesBuffer & rhs);


	int    mFront  = dfSERIES_BUF_DEFAULT_HEAD_START;
	int    mRear   = dfSERIES_BUF_DEFAULT_PAYLOAD_START;
	int    mBufferSize;
	int    mPayloadStart = dfSERIES_BUF_DEFAULT_PAYLOAD_START;
	int    mHeadSize     = dfSERIES_BUF_DEFAULT_HEAD_SIZE;
	int    mHeadStart    = dfSERIES_BUF_DEFAULT_HEAD_START;
	char * mBuffer       = nullptr;

	int    mRefCnt = 1;
};
