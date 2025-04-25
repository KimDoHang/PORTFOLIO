#pragma once

/*---------------------------------------------------
-----------------------------------------------------
#			LockFreeObjectPoolTLS (Release)
-----------------------------------------------------
----------------------------------------------------*/


//Node - Chunk - ChunkNode 순으로 관리가 이루어지며 ChunkNode의 경우 배열로 데이터를 가지고 있다.
//Node의 경우 하나의 리스트로 관리가 되며 ABA problem을 피하기 위해 마스킹 처리를 하고 있다.
//Chunk의 경우 ChunkNode의 생성자 호출을 막기 위해 malloc을 통해 할당이 이루어지며 초기화의 경우 인자와 ChunkNode 생성자 호출을 ChunkInitr 함수를 통해 해준다.
//ChunkNode의 경우 Replacement에 따라 초기 생성자 호출이 결정되어진다.
//마지막으로 소멸시에는 Replacement가 false인 경우에 한해서만 Chunk 소멸자 호출이 이루어지며 아닌 경우 항상 ChunkNode 소멸자가 호출된다.
//초기 Chunk의 수를 인자로 넘겨줄 수 있고 MAX_CHUNK_SIZE를 통해서 CHUNK 크기를 조절하는 것이 가능하다.


//template<typename DATA, bool Replacement>
//class LockFreeObjectPoolTLS
//{
//#pragma warning(disable: 6011)
//
//private:
//	static const int MONITOR_CHUNK_SIZE = 10;
//	static const uint64 COOKIE_ERASE = 0xeeee'eeee'eeee'eeee;
//	static const uint64 BITMASK_SHIFT47 = 47;
//	static const uint64 MASK_POINTER64 = (((uint64)1 << 47) - 1);
//
//	class Chunk;
//
//#ifdef TLS_POOL_LOG_LEVEL
//	struct ChunkNode
//	{
//		Chunk* _chunk;
//		size_t _cookie_front;
//		DATA _data;
//		size_t _cookie_back;
//	};
//#else
//	struct ChunkNode
//	{
//		Chunk* _chunk;
//		DATA _data;
//	};
//
//#endif
//	class Chunk
//	{
//	public:
//		//소멸자가 호출되는지 확인하자
//		Chunk() = default;
//		~Chunk() = default;
//
//		template<typename... Args>
//		__forceinline DATA* ChunkNodeAlloc(Args&&... args)
//		{
//			//interlock이 아니여도 괜찮은가??
//#ifdef TLS_POOL_LOG_LEVEL
//			InitAllocInfo(chunk_node);
//#endif
//			if constexpr (Replacement)
//				new (&_chunk_nodes[_alloc_cnt]._data) DATA(std::forward<Args>(args)...);
//
//			return &_chunk_nodes[_alloc_cnt]._data;
//		}
//
//		__forceinline LONG ChunkNodeFree(ChunkNode* chunk_node)
//		{
//#ifdef TLS_POOL_LOG_LEVEL
//			CheckDebugInfo(chunk_node);
//			InitReleaseInfo(chunk_node);
//#endif
//			if constexpr (Replacement)
//				chunk_node->_data.~DATA();
//
//			return InterlockedIncrement(&_free_cnt);
//		}
//
//		__forceinline void ChunkClear()
//		{
//			_alloc_cnt = 0;
//			_free_cnt = 0;
//		}
//
//		template<typename... Args>
//		__forceinline void ChunkInit(Args&&... args)
//		{
//			for (int i = 0; i < MAX_CHUNK_SIZE; i++)
//			{
//				_chunk_nodes[i]._chunk = this;
//
//				if constexpr (Replacement == false)
//				{
//					new (&_chunk_nodes[i]._data) DATA(std::forward<Args>(args)...);
//				}
//			}
//		}
//
//		//Debug Mode 
//#ifdef TLS_POOL_LOG_LEVEL
//		__forceinline void InitAllocInfo(ChunkNode* chunk_node)
//		{
//			chunk_node->_cookie_front = reinterpret_cast<size_t>(this);
//			chunk_node->_cookie_back = reinterpret_cast<size_t>(this);
//		}
//
//		__forceinline void InitReleaseInfo(ChunkNode* chunk_node)
//		{
//			chunk_node->_cookie_front = COOKIE_ERASE;
//			chunk_node->_cookie_back = COOKIE_ERASE;
//		}
//
//		__forceinline void CheckDebugInfo(ChunkNode* chunk_node)
//		{
//			if (chunk_node->_cookie_front != reinterpret_cast<size_t>(this) || chunk_node->_cookie_back != reinterpret_cast<size_t>(this))
//			{
//				if (chunk_node->_cookie_front == COOKIE_ERASE || chunk_node->_cookie_back == COOKIE_ERASE)
//				{
//					__debugbreak();
//				}
//				else
//				{
//					//std::wcout << L"ChunkNode corruption " << &chunk_node->_data << '\n';
//					__debugbreak();
//				}
//			}
//
//		}
//#endif
//		LONG _alloc_cnt;
//		ChunkNode _chunk_nodes[MAX_CHUNK_SIZE];
//		LONG _free_cnt;
//	};
//
//	struct st_NODE
//	{
//		st_NODE* nextNode;
//		Chunk _chunk;
//	};
//
//
//public:
//
//	template<typename... Args>
//	LockFreeObjectPoolTLS(int chunk_cnt = 0, Args&&... args) : _chunk_capacity(0), _chunk_use_cnt(0), _chunk_release_cnt(0), _chunknode_use_cnt(0), _chunknode_release_cnt(0), _cmpBit(0)
//	{
//		_top_free = nullptr;
//		st_NODE* newNode;
//
//		//언제나 Chunk는 ChunkNode 때문에 
//		for (int cnt = 0; cnt < chunk_cnt; cnt++)
//		{
//			newNode = (st_NODE*)malloc(sizeof(st_NODE));
//			newNode->nextNode = _top_free;
//			newNode->_chunk.ChunkInit(std::forward<Args>(args)...);
//			_top_free = MaskNode(newNode);
//		}
//
//		_chunk_capacity = chunk_cnt;
//	}
//
//	~LockFreeObjectPoolTLS()
//	{
//		st_NODE* tempNode;
//
//		while (_top_free != nullptr)
//		{
//			tempNode = UnMaskNode(_top_free);
//			_top_free = tempNode->nextNode;
//
//			//false인 경우만 소멸자를 통해 ChunkNode의 소멸자를 호출해준다.
//			if constexpr (Replacement == false)
//				tempNode->_chunk.~Chunk();
//
//			free(tempNode);
//		}
//	}
//
//	template<typename... Args>
//	DATA* Alloc(Args&&... args)
//	{
//		if (_tls_chunk == nullptr)
//		{
//			//Replacement가 false라면 한번만 초기화이므로 실제 할당시에 초기화가 이루어진다.
//			if (Replacement == false)
//				_tls_chunk = ChunkAlloc(std::forward<Args>(args)...);
//			else
//				_tls_chunk = ChunkAlloc();
//
//			_tls_chunk->ChunkClear();
//			InterlockedAdd(reinterpret_cast<long*>(&_chunknode_release_cnt), MAX_CHUNK_SIZE);
//		}
//
//		Chunk* tls_chunk = _tls_chunk;
//		DATA* data;
//
//		//true인 경우만 인자를 넘겨 초기화가 진행된다.
//		if (Replacement)
//			data = tls_chunk->ChunkNodeAlloc(std::forward<Args>(args)...);
//		else
//			data = tls_chunk->ChunkNodeAlloc();
//
//
//		//다 사용시 nullptr로 초기화 이때 싱글이므로 문제가 되지 않는다.
//		if (++tls_chunk->_alloc_cnt == MAX_CHUNK_SIZE)
//		{
//			_tls_chunk = nullptr;
//		}
//
//		InterlockedIncrement(&_chunknode_use_cnt);
//		InterlockedDecrement(&_chunknode_release_cnt);
//
//		return data;
//	}
//
//	void Free(DATA* p_data)
//	{
//		ChunkNode* chunk_node = reinterpret_cast<ChunkNode*>(reinterpret_cast<size_t>(p_data) - offsetof(ChunkNode, _data));
//
//		//순서에 따라 순번이 느린 Chunk가 먼저 처리될 수 있지만 아래의 코드가 처리되지 않으므로 문제가 되지 않는다.
//		if (chunk_node->_chunk->ChunkNodeFree(chunk_node) == MAX_CHUNK_SIZE)
//		{
//			InterlockedAdd(reinterpret_cast<long*>(&_chunknode_release_cnt), MAX_CHUNK_SIZE * -1);
//			ChunkFree(chunk_node->_chunk);
//		}
//		//Chunknode 자체를 건드리는 것이 아니므로 아래에서 처리해도 상관 X
//		InterlockedDecrement(&_chunknode_use_cnt);
//		InterlockedIncrement(&_chunknode_release_cnt);
//	}
//
//	uint32 GetChunkCapacityCount()
//	{
//		return _chunk_capacity;
//	}
//
//	uint32 GetChunkUseCount()
//	{
//		return _chunk_use_cnt;
//	}
//
//	uint32 GetChunkReleaseCount()
//	{
//		return _chunk_release_cnt;
//	}
//
//	uint32 GetChunkNodeUseCount()
//	{
//		return _chunknode_use_cnt;
//	}
//
//	uint32 GetChunkNodeReleaseCount()
//	{
//		return _chunknode_release_cnt;
//	}
//private:
//
//	template<typename... Args>
//	Chunk* ChunkAlloc(Args&&... args)
//	{
//		st_NODE* outNode;
//		st_NODE* cmpNode;
//
//		while (true)
//		{
//			//현재 Top Node 확인
//			cmpNode = _top_free;
//
//			//NULL이면 없으므로 그냥 비교 노드에 할당 받아 반환
//			if (cmpNode == nullptr)
//			{
//				cmpNode = (st_NODE*)malloc(sizeof(st_NODE));
//
//				//Node - Chunk - ChunkNode의 생성자 연쇄 호출을 막기 위해 malloc으로 할당 뒤, 각 ChunkNode의 주소 값과 false일 경우 생성자를 호출해주는 초기화 함수이다.
//				cmpNode->_chunk.ChunkInit(std::forward<Args>(args)...);
//				InterlockedIncrement(&_chunk_use_cnt);
//				InterlockedIncrement(&_chunk_capacity);
//				return  &cmpNode->_chunk;
//			}
//
//			//있으므로 원래 주소로 변경, CMP 할 주소를 두어야 함
//			outNode = UnMaskNode(cmpNode);
//
//			//어차피 outNode는 nextNode는 Memory Pool이므로 사라지지 않는다. 따라서 그냥 바로 사용
//			//만약 두 unique한 주소가 동일하다면 (유일한 노드) -> 비교 후 break 해당 노드는 이제 나만 건드릴 수 있으므로 data를 반환해준다.
//			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_top_free), outNode->nextNode, cmpNode) == cmpNode)
//			{
//				break;
//			}
//		}
//
//		//Replacement 적용 및 사용 갯수 추가
//		InterlockedIncrement(&_chunk_use_cnt);
//		InterlockedDecrement(&_chunk_release_cnt);
//
//		return &outNode->_chunk;
//	}
//
//	//어차피 이중 delete는 에러 대상이므로 누군가 한번만 호출한다고 고려해야 한다.
//	void	ChunkFree(Chunk* pData)
//	{
//		st_NODE* freeNode;
//		st_NODE* top;
//		st_NODE* tempNode = BringOrignNode(pData);
//
//		freeNode = MaskNode(tempNode);
//
//		while (true)
//		{
//			top = _top_free;
//			tempNode->nextNode = top;
//
//			//tempNode->next를 바로 사용하지 못하는 이유는 interlocked 하는 순간 언제나 next는 달라질 수 있으므로 비교시에는 고정값이 필요하다.
//			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_top_free), freeNode, top) == top)
//			{
//				break;
//			}
//		}
//
//		InterlockedDecrement(&_chunk_use_cnt);
//		InterlockedIncrement(&_chunk_release_cnt);
//	}
//
//	__forceinline st_NODE* MaskNode(st_NODE* node)
//	{
//
//		uint64 cmpBit = _InterlockedIncrement(reinterpret_cast<unsigned long long*>(&_cmpBit));
//		cmpBit = cmpBit << BITMASK_SHIFT47;
//		return reinterpret_cast<st_NODE*>((reinterpret_cast<uint64>(node) | cmpBit));
//	}
//
//	__forceinline st_NODE* UnMaskNode(st_NODE* node)
//	{
//		return 	reinterpret_cast<st_NODE*>(reinterpret_cast<uint64>(node) & MASK_POINTER64);
//	}
//
//	__forceinline st_NODE* BringOrignNode(Chunk* pData)
//	{
//		return reinterpret_cast<st_NODE*>(reinterpret_cast<uint64>(pData) - offsetof(st_NODE, _chunk));
//	}
//
//private:
//	static thread_local Chunk* _tls_chunk;
//	alignas(64) uint64 _cmpBit;
//	alignas(64) st_NODE* _top_free;
//	alignas(64) long _chunk_capacity;
//	alignas(64) long _chunk_use_cnt;
//	alignas(64) long _chunk_release_cnt;
//	alignas(64) long _chunknode_use_cnt;
//	alignas(64) long _chunknode_release_cnt;
//};
//
//template<typename DATA, bool Replacement>
//thread_local typename LockFreeObjectPoolTLS<DATA, Replacement>::Chunk* LockFreeObjectPoolTLS<DATA, Replacement>::_tls_chunk = nullptr;


template<typename DATA, bool Replacement>
class LockFreeObjectPoolTLS
{
#pragma warning(disable: 6011)

private:
	static const int MONITOR_CHUNK_SIZE = 100;
	static const uint64 COOKIE_ERASE = 0xeeee'eeee'eeee'eeee;
	static const uint64 BITMASK_SHIFT47 = 47;
	static const uint64 MASK_POINTER64 = (((uint64)1 << 47) - 1);

	class Chunk;

#ifdef TLS_POOL_LOG_LEVEL
	struct ChunkNode
	{
		Chunk* _chunk;
		size_t _cookie_front;
		DATA _data;
		size_t _cookie_back;
	};
#else
	struct ChunkNode
	{
		Chunk* _chunk;
		DATA _data;
	};

#endif
	class Chunk
	{
	public:
		//소멸자가 호출되는지 확인하자
		Chunk() = default;
		~Chunk() = default;

		template<typename... Args>
		__forceinline DATA* ChunkNodeAlloc(Args&&... args)
		{
			//interlock이 아니여도 괜찮은가??
#ifdef TLS_POOL_LOG_LEVEL
			InitAllocInfo(chunk_node);
#endif
			if constexpr (Replacement)
				new (&_chunk_nodes[_alloc_cnt]._data) DATA(std::forward<Args>(args)...);

			return &_chunk_nodes[_alloc_cnt]._data;
		}

		__forceinline LONG ChunkNodeFree(ChunkNode* chunk_node)
		{
#ifdef TLS_POOL_LOG_LEVEL
			CheckDebugInfo(chunk_node);
			InitReleaseInfo(chunk_node);
#endif
			if constexpr (Replacement)
				chunk_node->_data.~DATA();

			return InterlockedIncrement(&_free_cnt);
		}

		__forceinline void ChunkClear()
		{
			_alloc_cnt = 0;
			_free_cnt = 0;
		}

		template<typename... Args>
		__forceinline void ChunkInit(Args&&... args)
		{
			for (int i = 0; i < MAX_CHUNK_SIZE; i++)
			{
				_chunk_nodes[i]._chunk = this;

				if constexpr (Replacement == false)
				{
					new (&_chunk_nodes[i]._data) DATA(std::forward<Args>(args)...);
				}
			}
		}

		//Debug Mode 
#ifdef TLS_POOL_LOG_LEVEL
		__forceinline void InitAllocInfo(ChunkNode* chunk_node)
		{
			chunk_node->_cookie_front = reinterpret_cast<size_t>(this);
			chunk_node->_cookie_back = reinterpret_cast<size_t>(this);
		}

		__forceinline void InitReleaseInfo(ChunkNode* chunk_node)
		{
			chunk_node->_cookie_front = COOKIE_ERASE;
			chunk_node->_cookie_back = COOKIE_ERASE;
		}

		__forceinline void CheckDebugInfo(ChunkNode* chunk_node)
		{
			if (chunk_node->_cookie_front != reinterpret_cast<size_t>(this) || chunk_node->_cookie_back != reinterpret_cast<size_t>(this))
			{
				if (chunk_node->_cookie_front == COOKIE_ERASE || chunk_node->_cookie_back == COOKIE_ERASE)
				{
					__debugbreak();
				}
				else
				{
					//std::wcout << L"ChunkNode corruption " << &chunk_node->_data << '\n';
					__debugbreak();
				}
			}

		}
#endif
		LONG _alloc_cnt;
		LONG _free_cnt;
		ChunkNode _chunk_nodes[MAX_CHUNK_SIZE];
	};

	struct st_NODE
	{
		st_NODE* nextNode;
		Chunk _chunk;
	};


public:

	template<typename... Args>
	LockFreeObjectPoolTLS(int chunk_cnt = 0, Args&&... args) : _chunk_capacity(0), _chunk_use_cnt(0), _chunk_release_cnt(0), _chunknode_use_cnt(0), _chunknode_release_cnt(0), _cmp_bit(0)
	{
		st_NODE* temp_node = (st_NODE*)malloc(sizeof(st_NODE));
		_head = _tail = MaskNode(temp_node);

		//Replacement true이면 어차피 인자 X, false라면 넘겨주어야 한다.
		temp_node->_chunk.ChunkInit(std::forward<Args>(args)...);
		//언제나 Chunk는 ChunkNode 때문에 
		for (int cnt = 0; cnt < chunk_cnt; cnt++)
		{
			temp_node->nextNode = (st_NODE*)malloc(sizeof(st_NODE));
			temp_node = temp_node->nextNode;
			temp_node->_chunk.ChunkInit(std::forward<Args>(args)...);
			_tail = MaskNode(temp_node);
		}

		temp_node->nextNode = nullptr;
		_chunk_capacity = chunk_cnt;
	}

	~LockFreeObjectPoolTLS()
	{
		st_NODE* temp_node;

		while (true)
		{
			temp_node = UnMaskNode(_head);

			if (temp_node->nextNode == nullptr)
				break;
			//false인 경우만 소멸자를 통해 ChunkNode의 소멸자를 호출해준다.
			if constexpr (Replacement == false)
				temp_node->_chunk.~Chunk();

			_head = temp_node->nextNode;

			free(temp_node);
		}

		if constexpr (Replacement == false)
			temp_node->_chunk.~Chunk();

		free(temp_node);
	}

	template<typename... Args>
	DATA* Alloc(Args&&... args)
	{
		if (_tls_chunk == nullptr)
		{
			//Replacement가 false라면 한번만 초기화이므로 실제 할당시에 초기화가 이루어진다.
			if constexpr (Replacement == false)
				_tls_chunk = ChunkAlloc(std::forward<Args>(args)...);
			else
				_tls_chunk = ChunkAlloc();

			_tls_chunk->ChunkClear();
			InterlockedAdd(reinterpret_cast<long*>(&_chunknode_release_cnt), MAX_CHUNK_SIZE);
		}

		Chunk* tls_chunk = _tls_chunk;

		DATA* data;

		//true인 경우만 인자를 넘겨 초기화가 진행된다.
		if constexpr (Replacement)
			data = tls_chunk->ChunkNodeAlloc(std::forward<Args>(args)...);
		else
			data = tls_chunk->ChunkNodeAlloc();


		//다 사용시 nullptr로 초기화 이때 싱글이므로 문제가 되지 않는다.
		if (++tls_chunk->_alloc_cnt == MAX_CHUNK_SIZE)
		{
			_tls_chunk = nullptr;
		}

		InterlockedIncrement(&_chunknode_use_cnt);
		InterlockedDecrement(&_chunknode_release_cnt);

		return data;
	}

	void Free(DATA* p_data)
	{
		ChunkNode* chunk_node = reinterpret_cast<ChunkNode*>(reinterpret_cast<size_t>(p_data) - offsetof(ChunkNode, _data));

		if (chunk_node->_chunk->ChunkNodeFree(chunk_node) == MAX_CHUNK_SIZE)
		{
			ChunkFree(chunk_node->_chunk);
			InterlockedAdd(reinterpret_cast<long*>(&_chunknode_release_cnt), MAX_CHUNK_SIZE * -1);
		}

		InterlockedDecrement(&_chunknode_use_cnt);
		InterlockedIncrement(&_chunknode_release_cnt);
	}

	__forceinline uint32 GetChunkCapacityCount()
	{
		return _chunk_capacity;
	}

	__forceinline uint32 GetChunkUseCount()
	{
		return _chunk_use_cnt;
	}

	__forceinline uint32 GetChunkReleaseCount()
	{
		return _chunk_release_cnt;
	}

	__forceinline uint32 GetChunkNodeUseCount()
	{
		return _chunknode_use_cnt;
	}

	__forceinline uint32 GetChunkNodeReleaseCount()
	{
		return _chunknode_release_cnt;
	}
private:

	template<typename... Args>
	Chunk* ChunkAlloc(Args&&... args)
	{
		st_NODE* mask_tail;
		st_NODE* tail;
		st_NODE* mask_head;
		st_NODE* head;

		long size = _InterlockedDecrement(&_chunk_release_cnt);

		//release_cnt 숫자를 보고 없다면 할당헤서 나간다. 이때 해당 노드의 Chunk도 사용되므로 초기화가 필요하다.
		//next null의 경우 queue에 연결하는 경우만 필요하므로 그냥 초기화 하지 않는다.
		if (size < 0)
		{
			_InterlockedIncrement(&_chunk_release_cnt);
			head = (st_NODE*)malloc(sizeof(st_NODE));
			//Node - Chunk - ChunkNode의 생성자 연쇄 호출을 막기 위해 malloc으로 할당 뒤, 각 ChunkNode의 주소 값과 false일 경우 생성자를 호출해주는 초기화 함수이다.
			head->_chunk.ChunkInit(std::forward<Args>(args)...);
			InterlockedIncrement(&_chunk_use_cnt);
			InterlockedIncrement(&_chunk_capacity);
			return  &head->_chunk;
		}

		while (true)
		{
			mask_tail = _tail;
			tail = UnMaskNode(mask_tail);

			//만약 연결되어 있지 않다면 연결한다. 이때 tail->next의 경우 tail이 변경되지 않았다면 항상 동일하므로 굳이 저장하지 않는다.
			if (tail->nextNode != nullptr)
			{
				InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_tail), tail->nextNode, mask_tail);
				continue;
			}


			//지금은 data를 사용하는 것이 아닌 현재 노드 자체를 사용하므로 해당 노드를 반환해주면 된다.
			mask_head = _head;
			head = UnMaskNode(mask_head);

			//다른 스레드에서 제거하였거나 붕뜨는 상황이라 연결되지 않았다면 발생 가능한 상황이다.
			if (head->nextNode == nullptr)
				continue;

			//마찬가지로 head->next는 head가 변하지 않았다면 동일하므로 head->next로 변경하고 나가준다.
			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_head), head->nextNode, mask_head) == mask_head)
			{
				break;
			}

		}
		//Replacement 적용 및 사용 갯수 추가
		InterlockedIncrement(&_chunk_use_cnt);

		//어차피 데이터가 아닌 해당 node가 필요하므로 굳이 data를 복사하지 않고 해당 노드를 꺼내 사용한다.
		return &head->_chunk;
	}

	//어차피 이중 delete는 에러 대상이므로 누군가 한번만 호출한다고 고려해야 한다.
	void	ChunkFree(Chunk* pData)
	{
		st_NODE* free_node = BringOrignNode(pData);
		st_NODE* mask_free_node;
		st_NODE* tail;
		st_NODE* mask_tail;

		free_node->nextNode = nullptr;
		mask_free_node = MaskNode(free_node);

		while (true)
		{
			mask_tail = _tail;
			tail = UnMaskNode(mask_tail);

			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&tail->nextNode), mask_free_node, nullptr) == nullptr)
			{
				InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_tail), mask_free_node, mask_tail);
				break;
			}

			InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_tail), tail->nextNode, mask_tail);
		}

		InterlockedIncrement(&_chunk_release_cnt);
		InterlockedDecrement(&_chunk_use_cnt);
	}

	__forceinline st_NODE* MaskNode(st_NODE* node)
	{
		uint64 cmpBit = _InterlockedIncrement(reinterpret_cast<unsigned long long*>(&_cmp_bit));
		cmpBit = cmpBit << BITMASK_SHIFT47;
		return reinterpret_cast<st_NODE*>((reinterpret_cast<uint64>(node) | cmpBit));
	}

	__forceinline st_NODE* UnMaskNode(st_NODE* node)
	{
		return 	reinterpret_cast<st_NODE*>(reinterpret_cast<uint64>(node) & MASK_POINTER64);
	}

	__forceinline st_NODE* BringOrignNode(Chunk* pData)
	{
		return reinterpret_cast<st_NODE*>(reinterpret_cast<uint64>(pData) - offsetof(st_NODE, _chunk));
	}

private:
	static thread_local Chunk* _tls_chunk;
	alignas(64) uint64 _cmp_bit;
	alignas(64) st_NODE* _head;
	alignas(64) st_NODE* _tail;
	alignas(64) long _chunk_release_cnt;
	alignas(64) long _chunk_capacity;
	alignas(64) long _chunk_use_cnt;
	alignas(64) long _chunknode_use_cnt;
	alignas(64) long _chunknode_release_cnt;
};

template<typename DATA, bool Replacement>
thread_local typename LockFreeObjectPoolTLS<DATA, Replacement>::Chunk* LockFreeObjectPoolTLS<DATA, Replacement>::_tls_chunk = nullptr;

