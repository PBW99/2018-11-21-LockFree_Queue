#pragma once

class CRingBuffer
{
public:
	enum DEF
	{
		EMPTY_BYTE = 1
	};

	CRingBuffer(int queueSize)
		: mQueueSize(queueSize + EMPTY_BYTE)
	{
		mBuffer = new char[queueSize + EMPTY_BYTE];
	}

	~CRingBuffer()
	{
		delete[] mBuffer;
	}
	/**
	 * \brief 
	 * \param buffer 
	 * \param size 
	 * \return Success : 0, Fail : -1
	 */
	int Enqueue(IN char * buffer, IN int size);
	/**
	 * \brief 
	 * \param buffer 
	 * \param size 
	 * \return Success : 0, Fail : -1
	 */
	int Dequeue(OUT char * buffer, IN int size);
	int Peek(OUT char *    buffer, IN int size);

	/**
	* 현재 사용중인 용량 얻기.
	*
	* Parameters: 없음.
	* Return: (int)사용중인 용량.
	*/
	int GetUseSize(void) const;

	/**
	* 현재 버퍼에 남은 용량 얻기.
	*
	* Parameters: 없음.
	* Return: (int)남은용량.
	*/
	int GetFreeSize(void) const;

	/**
	* 버퍼 포인터로 외부에서 한방에 읽고, 쓸 수 있는 길이.
	* (끊기지 않은 길이)
	*
	* 원형 큐의 구조상 버퍼의 끝단에 있는 데이터는 끝 -> 처음으로 돌아가서
	* 2번에 데이터를 얻거나 넣을 수 있음. 이 부분에서 끊어지지 않은 길이를 의미
	*
	* Parameters: 없음.
	* Return: (int)사용가능 용량.
	*/

	int GetNotBrokenGetSize(void);
	int GetNotBrokenPutSize(void);

	/**

	* 원하는 길이만큼 읽기위치 에서 삭제 / 쓰기 위치 이동
	*
	* Parameters: iSize(무조건 Enqueue,Dequeue에서 검증된 사이즈여야함).
	* Return: 없음.
	*/
	void MoveRear(int  iSize);
	void MoveFront(int iSize);

	/**

	* 버퍼의 모든 데이타 삭제.
	*
	* Parameters: 없음.
	* Return: 없음.
	*/

	void ClearBuffer(void);

	/**

	* 버퍼의 포인터 얻음.
	*
	* Parameters: 없음.
	* Return: (char *) 버퍼 포인터.
	*/
	char* GetBufferPtr(void);

	/**
	* 버퍼의 Front 포인터 얻음.
	*
	* Parameters: 없음.
	* Return: (char *) 버퍼 포인터.
	*/
	char* GetFrontBufferPtr(void);

	/**
	* 버퍼의 RearPos 포인터 얻음.
	*
	* Parameters: 없음.
	* Return: (char *) 버퍼 포인터.
	*/
	char* GetRearBufferPtr(void);

	int GetFront();
	int GetRear();

private:
	volatile int    mFront = 0;
	volatile int    mRear= 0;
	int    mQueueSize;
	char * mBuffer;
};

