#pragma once

#define NOT_NULL_DELIMITER 0x1122334455667788

namespace LF64
{
	template <typename T>
	class QueueT
	{
	private:


		struct Node
		{
			T      data;
			Node * next = (Node *)NOT_NULL_DELIMITER;
		};

		typedef struct _declspec(align(16)) NodeABA
		{
			int64_t aba;
			Node *  node;
		}           NodeABA;


		NodeABA                   _head;        // 시작노드
		NodeABA                   _tail;        // 마지막노드.
		LF64::CMemoryPool<Node> * _node_pool;
		long                      _size;



	public:
		QueueT()
		{
			_node_pool = new LF64::CMemoryPool<Node>(0, false);
			_size      = 0;

			_head.aba        = 0;
			_head.node       = _node_pool->Alloc();
			_head.node->next = NULL;

			_tail.aba  = 0;
			_tail.node = _head.node;
		}

		virtual ~QueueT()
		{
			delete _node_pool;
		}


		long GetSize() const
		{
			return _size;
		}

		void Enqueue(T t)
		{
			Node * node;
			Node * next;

			NodeABA  tail;
			LONG64 * ptail = (LONG64*)&tail;

			NodeABA  newtail;
			LONG64 * pnewtail = (LONG64*)&newtail;

			// Tail.next follow
			Node * cur;

			NodeABA  lasttail;
			LONG64 * plasttail = (LONG64*)&lasttail;

			node       = _node_pool->Alloc();
			node->data = t;
			node->next = NULL;

			while (true)
			{
				tail.aba  = _tail.aba;
				tail.node = _tail.node;

				next = tail.node->next;

				newtail.aba  = tail.aba + 1;
				newtail.node = node;


				if (next == NULL)
				{
					if (InterlockedCompareExchangePointer((PVOID*)&tail
					                                               .node->next,
					                                      node,
					                                      NULL) == NULL)
					{
						InterlockedCompareExchange128((LONG64*)&_tail,
						                              *(pnewtail + 1),
						                              *(pnewtail),
						                              ptail);
						break;
					}
				}
				else
				{
					// Follow tail.next
					cur = next;
					while (tail.node == _tail.node && cur && cur->next != NULL)
					{
						cur = cur->next;
					}
					if (tail.node == _tail.node && cur && cur->next == NULL)
						// if (tail.node == _tail.node && cur)
					{
						lasttail.aba  = tail.aba + 1;
						lasttail.node = cur;

						InterlockedCompareExchange128((LONG64*)&_tail,
						                              *(plasttail + 1),
						                              *(plasttail),
						                              ptail);
					}
				}
			}


			InterlockedExchangeAdd(&_size, 1);
		}

		int Dequeue(T & t)
		{
			Node * next;

			NodeABA  head;
			LONG64 * phead = (LONG64*)&head;

			NodeABA  newhead;
			LONG64 * pnewhead = (LONG64*)&newhead;

			// Tail.next follow
			Node * cur;

			NodeABA  tail;
			LONG64 * ptail = (LONG64*)&tail;

			NodeABA  lasttail;
			LONG64 * plasttail = (LONG64*)&lasttail;

			if (_size > 0)
			{
				if (InterlockedDecrement(&_size) < 0)
				{
					InterlockedIncrement(&_size);
					return -1;
				}
			}
			else
			{
				return -1;
			}

			while (true)
			{
				head.aba  = _head.aba;
				head.node = _head.node;

				next = head.node->next;

				newhead.aba  = head.aba + 1;
				newhead.node = next;

				if (next == NULL)
				{
					continue;
				}
				else
				{
					tail.aba  = _tail.aba;
					tail.node = _tail.node;
					if (head.node == tail.node)
					{
						
						// Follow tail.next
						cur = head.node->next;
						while (head.node == tail.node && cur && cur->next !=
							NULL)
						{
							tail.aba  = _tail.aba;
							tail.node = _tail.node;
							cur       = cur->next;
						}
						if (head.node == tail.node && cur && cur->next == NULL)
						{
							lasttail.aba  = tail.aba + 1;
							lasttail.node = cur;

							InterlockedCompareExchange128((LONG64*)&_tail,
							                              *(plasttail + 1),
							                              *(plasttail),
							                              ptail);
						}
						continue;
					}

					if (InterlockedCompareExchange128((LONG64*)&_head,
					                                  *(pnewhead + 1),
					                                  *(pnewhead),
					                                  phead))
					{
						t = next->data;
						_node_pool->Free(head.node);
						break;
					}
				}
			}
			return 0;
		}
	};
}
