//64 bit Version!!
namespace LF64
{
	template <typename T>
	class CMemoryPool
	{
		static const int kAllocDelimeter = 0x05050505;
		// Children Types
	public:
		struct Node;
		typedef Node * NodePtr;

		typedef struct Node
		{
			NodePtr next = kAllocDelimeter;
			T       data;		//멤버로 가짐에 유의(free delete시 함께작동)
		}           Node;

		typedef struct _declspec(align(64)) NodeHead
		{
			uintptr_t aba  = 0;
			NodePtr   node = (NodePtr)kAllocDelimeter;
		}             NodeHead;

		class CMemoryPoolException : public std::exception
		{
		public:

			CMemoryPoolException(bool isFreeWrongArgument)
				: isFreeWrongArgument(isFreeWrongArgument)
			{
				mWhatMsg << "[CMemoryPoolException]\n";
				mWhatMsg << "isFreeWorongArgument : " << isFreeWrongArgument <<
					"\n";
			}

			virtual char const* what() const override
			{
				return mWhatMsg.str().c_str();
			};
		private:
			static const int   kMsgSize = 1000;
			bool               isFreeWrongArgument;
			std::ostringstream mWhatMsg;
		};


		// Members
	public:

		CMemoryPool(int initialPoolSize, bool isOnPlacementNew)
			: mInitialPoolSize(initialPoolSize),
			  isOnPlacementNew(isOnPlacementNew)
		{
			InitializeSRWLock(&mLock);
			CreateNode(mInitialPoolSize);
		};

		virtual ~CMemoryPool()
		{
			DeleteFreeNodeList();
		};

		T* Alloc()
		{
			NodePtr front = PopFreeNode();
			InterlockedIncrement((long*)&mUseCount);
			if (front == nullptr)
			{
				// 새로 할당한 node의 next를 kAllocDelimeter로 설정
				NodePtr node = (NodePtr)malloc(sizeof(Node));
				node->next   = (NodePtr)kAllocDelimeter;

				// 생성자 호출
				new(&node->data) T;

				InterlockedIncrement((long*)&mCreateCount);
				return &node->data;
			}
			front->next = (NodePtr)kAllocDelimeter;

			//이미 프리노드가 존재
			if (isOnPlacementNew)
			{
				// 생성자 호출
				new(&front->data) T;
			}

			return &front->data;
		};

		void Free(T * data)
		{
			if (!CheckNodeCorrection(data)) throw CMemoryPoolException(true);

			NodePtr pTargetNode = (NodePtr)((char*)data - sizeof(NodePtr));

			if (isOnPlacementNew)
			{
				// 파괴자 호출
				(&pTargetNode->data)->~T();
			}
			PushFreeNode(pTargetNode);
			InterlockedDecrement((long*)&mUseCount);
		}

		int GetCreateCount() const
		{
			return mCreateCount;
		}

		int GetUseCount() const
		{
			return mUseCount;
		};


		friend std::ostream& operator<<(std::ostream &      os,
		                                const CMemoryPool & obj)
		{
			return os
				<< "mInitialPoolSize: " << obj.mInitialPoolSize
				<< "\n mCreateCount: " << obj.mCreateCount
				<< "\n mUseCount: " << obj.mUseCount
				// << "\n mTopFreeNode: " << obj.mTopFreeNode
				<< "\n mTopFreeNode: " << obj.mTopFreeNode.node
				<< "\n isOnPlacementNew: " << obj.isOnPlacementNew
				<< "\n";
		}

	private:
		/**
		 * \brief FreeNodeList 초기화
		 * \n<b>Entry:</b>\n  CMemoryPool(int , bool)
		 * \param num
		 */
		void CreateNode(int num)
		{
			for (int a = 0; a < num; a++)
			{
				// 새로 할당한 node의 next를 kAllocDelimeter로 설정
				NodePtr node = (NodePtr)malloc(sizeof(Node));
				node->next   = (NodePtr)kAllocDelimeter;


				new(&node->data) T;

				PushFreeNode(node);
			}
			mCreateCount += num;
		};

		/**
		 * \brief
		 * \n<b>Entry:</b>\n CreateNode(), Free()에서 호출
		 * \param node
		 */
		void PushFreeNode(NodePtr node)
		{
			// LF
			NodeHead nextH;
			NodeHead origH;
			long long *                   pnextH = nullptr;
			long long *                   porigH = nullptr;
			int                           cnt    = 1;
			do
			{
				origH.aba = mTopFreeNode.aba;
				origH.node = mTopFreeNode.node;
				node->next = origH.node;
				nextH.aba  = origH.aba + 1;
				nextH.node = node;
				pnextH     = (long long*)&nextH;
				porigH     = (long long*)&origH;
			}
			while (!InterlockedCompareExchange128((LONG64*)&mTopFreeNode,
			                                      *(pnextH + 1),
			                                      *(pnextH),
			                                      porigH));
		};

		/**
		 * \brief
		 * FreeNode를 pop하고,
		 * pop한 Node의 next를 kAllocDelimeter로 수정함
		 * \return
		 */
		NodePtr PopFreeNode()
		{

			//LF
			NodeHead nextH;
			NodeHead origH;
			long long *                   pnextH = nullptr;
			long long *                   porigH = nullptr;
			int                           cnt    = 1;
			do
			{
				origH.aba = mTopFreeNode.aba;
				origH.node= mTopFreeNode.node;
				if (origH.node == (NodePtr)kAllocDelimeter) return nullptr;
				nextH.aba  = origH.aba + 1;
				nextH.node = origH.node->next;
				pnextH     = (long long*)&nextH;
				porigH     = (long long*)&origH;
			}
			while (!InterlockedCompareExchange128((LONG64*)&mTopFreeNode,
			                                      *(pnextH + 1),
			                                      *(pnextH),
			                                      porigH));


			return origH.node;
		};

		/**
		 * \brief  data를 통해 Node를 찾고, 맞는 node->next를 통해 Correctness를 체크
		 * \param data
		 * \return
		 */
		bool CheckNodeCorrection(T * data)
		{
			NodePtr pTargetNode = (NodePtr)((char*)data - sizeof(NodePtr));
			NodePtr targetNext  = pTargetNode->next;
			if (targetNext == (NodePtr)kAllocDelimeter) return true;
			return false;
		}

		/**
		 * \brief  <b>FreeNodeList를 삭제함</b> \n
		 * isOnPlacementNew - On시에는 이미 Free()에서 파괴자가 호출되었으므로 메모리만 해제 \n
		 * Off시에는 파괴자도 같이 호출해줌(delete)
		 */
		void DeleteFreeNodeList()
		{
			NodePtr bef  = mTopFreeNode.node;
			NodePtr node = mTopFreeNode.node;
			while (node != (NodePtr)kAllocDelimeter)
			{
				bef  = node;
				node = node->next;
				if (isOnPlacementNew) free(bef);
				else delete bef;
			}
		}

		SRWLOCK  mLock;
		int      mInitialPoolSize;
		int      mCreateCount = 0;
		int      mUseCount    = 0;
		NodeHead mTopFreeNode;
		bool     isOnPlacementNew;
	};
};
