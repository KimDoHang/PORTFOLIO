#pragma once

#pragma warning(disable: 4101)

#define df_LIST_RELEASE_BLOCK_USE_CNT 1000
#define df_LIST_MAX_BLOCK_CNT 2000


template<typename DATA, bool Replacement>
class LockFreeObjectPoolTLSList
{
	//#pragma warning(disable: 6011)

private:
	static const int LIST_MONITOR_CHUNK_SIZE = 100;
	static const uint64 LIST_COOKIE_ERASE = 0xeeee'eeee'eeee'eeee;
	static const uint64 LIST_BITMASK_SHIFT47 = 47;
	static const uint64 LIST_MASK_POINTER64 = (((uint64)1 << 47) - 1);


#ifdef LIST_TLS_POOL_LOG_LEVEL
	struct Block
	{
		Block* next;
		size_t cookie_front;
		DATA data;
		size_t cookie_back;
	};
#else
	struct Block
	{
		Block* next_block;
		DATA data;
	};
#endif
	struct Node
	{
		Node* next_node;
		Block* block_top;
	};

	struct BlockHeader
	{
		Block* top = nullptr;
		int32 cnt = 0;
	};

public:

	template<typename... Args>
	LockFreeObjectPoolTLSList(Args&&... args) :_block_chunk_use_cnt(0), _block_chunk_release_cnt(0), _block_chunk_capacity_cnt(0), _node_capacity_cnt(0), _node_use_cnt(0), _node_release_cnt(0), _block_use_cnt(0), _block_release_cnt(0), _cmp_bit(0)
	{
		_block_pool_manager = HeapCreate(0, 0, 0);

		if (_block_pool_manager == NULL)
		{
			__debugbreak();
		}

		Node* node_lid = (Node*)malloc(sizeof(Node));
		node_lid->next_node = nullptr;
		_node_pool_head = _node_pool_tail = MaskNode(node_lid);

		node_lid = (Node*)malloc(sizeof(Node));
		node_lid->next_node = nullptr;
		_block_pool_head = _block_pool_tail = MaskNode(node_lid);

	}

	~LockFreeObjectPoolTLSList()
	{
		Node* block_pool_head;
		Node* node_pool_head;

		while (true)
		{
			block_pool_head = UnMaskNode(_block_pool_head);

			if (block_pool_head->next_node == nullptr)
				break;

			//false인 경우만 소멸자를 통해 ChunkNode의 소멸자를 호출해준다.
			if constexpr (Replacement == false)
				BlockChunkClear(block_pool_head->block_top);

			_block_pool_head = block_pool_head->next_node;
			NodeFree(block_pool_head);
		}

		HeapDestroy(_block_pool_manager);


		while (true)
		{
			node_pool_head = UnMaskNode(_node_pool_head);

			if (node_pool_head->next_node == nullptr)
				break;

			//false인 경우만 소멸자를 통해 ChunkNode의 소멸자를 호출해준다.

			_node_pool_head = node_pool_head->next_node;
			free(node_pool_head);
		}

		free(_block_pool_head);
		free(_node_pool_head);
	}

	template<typename... Args>
	DATA* Alloc(Args&&... args)
	{
		BlockHeader* alloc_header = &_tls_alloc_block_header;

		//BlockLid의 block들 할당 받기
		if (alloc_header->top == nullptr)
		{
			//Replacement가 false라면 한번만 초기화이므로 실제 할당시에 초기화가 이루어진다.
			BlockHeader* free_header = &_tls_free_block_header;

			if (free_header->cnt > df_LIST_RELEASE_BLOCK_USE_CNT)
			{
				alloc_header->top = free_header->top;
				alloc_header->cnt = free_header->cnt;
				free_header->top = nullptr;
				free_header->cnt = 0;
			}
			else
			{
				if constexpr (Replacement == false)
					alloc_header->top = BlockAlloc(std::forward<Args>(args)...);
				else
					alloc_header->top = BlockAlloc();

				InterlockedAdd(reinterpret_cast<long*>(&_block_release_cnt), df_LIST_MAX_BLOCK_CNT);
			}

		}

		//Block 옮기기
		Block* block = alloc_header->top;
		alloc_header->top = alloc_header->top->next_block;

		//true인 경우만 인자를 넘겨 초기화가 진행된다.
		if constexpr (Replacement)
			new(&block->data) DATA(std::forward<Args>(args)...);

		InterlockedIncrement(&_block_use_cnt);
		InterlockedDecrement(&_block_release_cnt);

		return &block->data;
	}

	void Free(DATA* p_data)
	{
		Block* block = reinterpret_cast<Block*>(reinterpret_cast<size_t>(p_data) - offsetof(Block, data));

		if constexpr (Replacement)
		{
			block->data.~DATA();
		}

		BlockHeader* free_header = &_tls_free_block_header;

		block->next_block = free_header->top;
		free_header->top = block;

		if ((++free_header->cnt) == df_LIST_MAX_BLOCK_CNT)
		{
			BlockFree(free_header->top);
			free_header->cnt = 0;
			free_header->top = nullptr;

			InterlockedAdd(&_block_release_cnt, df_LIST_MAX_BLOCK_CNT * -1);
		}

		_InterlockedDecrement(&_block_use_cnt);
		_InterlockedIncrement(&_block_release_cnt);
	}

	__forceinline int32 GetNodeCapacityCount()
	{
		return _node_capacity_cnt;
	}

	__forceinline int32 GetNodeUseCount()
	{
		return _node_use_cnt;
	}

	__forceinline int32 GetNodeReleaseCount()
	{
		return _node_release_cnt;
	}

	__forceinline int32 GetBlockChunkCapacityCount()
	{
		return _block_chunk_capacity_cnt;
	}

	__forceinline int32 GetBlockChunkUseCount()
	{
		return _block_chunk_use_cnt;
	}

	__forceinline int32 GetBlockChunkReleaseCount()
	{
		return _block_chunk_release_cnt;
	}

	__forceinline int32 GetBlockUseCount()
	{
		return _block_use_cnt;
	}

	__forceinline int32 GetBlockReleaseCount()
	{
		return _block_release_cnt;
	}


private:

	template<typename... Args>
	Block* BlockChunkInit(Args&&... args)
	{
		Block* blocks = reinterpret_cast<Block*>(HeapAlloc(_block_pool_manager, 0, sizeof(Block) * df_LIST_MAX_BLOCK_CNT));

		int idx;

		for (idx = 0; idx < df_LIST_MAX_BLOCK_CNT - 1; idx++)
		{
			blocks[idx].next_block = &blocks[idx + 1];
			if constexpr (Replacement == false)
			{
				new (&blocks[idx].data) DATA(std::forward<Args>(args)...);
			}
		}

		blocks[idx].next_block = nullptr;
		if constexpr (Replacement == false)
		{
			new (&blocks[idx].data) DATA(std::forward<Args>(args)...);
		}

		return blocks;
	}

	void BlockChunkClear(Block* blocks)
	{
		Block* clear_node;
		while (true)
		{
			if (blocks == nullptr)
				break;

			blocks->data.~DATA();
			blocks = blocks->next_block;
		}
	}

	template<typename... Args>
	Block* BlockAlloc(Args&&... args)
	{
		Node* mask_block_pool_tail;
		Node* block_pool_tail;
		Node* mask_block_pool_head;
		Node* block_pool_head;
		Node* mask_block_pool_head_next;
		Node* block_pool_head_next;
		Block* blocks;

		long size = _InterlockedDecrement(&_block_chunk_release_cnt);

		//release_cnt 숫자를 보고 없다면 할당헤서 나간다. 이때 해당 노드의 Chunk도 사용되므로 초기화가 필요하다.
		//next null의 경우 queue에 연결하는 경우만 필요하므로 그냥 초기화 하지 않는다.
		if (size < 0)
		{
			_InterlockedIncrement(&_block_chunk_release_cnt);

			blocks = BlockChunkInit(std::forward<Args>(args)...);
			//Node - Chunk - ChunkNode의 생성자 연쇄 호출을 막기 위해 malloc으로 할당 뒤, 각 ChunkNode의 주소 값과 false일 경우 생성자를 호출해주는 초기화 함수이다.
			_InterlockedIncrement(&_block_chunk_use_cnt);
			_InterlockedIncrement(&_block_chunk_capacity_cnt);
			return blocks;
		}

		while (true)
		{
			mask_block_pool_tail = _block_pool_tail;
			block_pool_tail = UnMaskNode(mask_block_pool_tail);

			//만약 연결되어 있지 않다면 연결한다. 이때 tail->next의 경우 tail이 변경되지 않았다면 항상 동일하므로 굳이 저장하지 않는다.
			if (block_pool_tail->next_node != nullptr)
			{
				InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_block_pool_tail), block_pool_tail->next_node, mask_block_pool_tail);
				continue;
			}

			//지금은 data를 사용하는 것이 아닌 현재 노드 자체를 사용하므로 해당 노드를 반환해주면 된다.
			mask_block_pool_head = _block_pool_head;
			block_pool_head = UnMaskNode(mask_block_pool_head);

			mask_block_pool_head_next = block_pool_head->next_node;
			block_pool_head_next = UnMaskNode(mask_block_pool_head_next);

			//다른 스레드에서 제거하였거나 붕뜨는 상황이라 연결되지 않았다면 발생 가능한 상황이다.
			if (block_pool_head_next == nullptr)
				continue;

			blocks = block_pool_head_next->block_top;

			//마찬가지로 head->next는 head가 변하지 않았다면 동일하므로 head->next로 변경하고 나가준다.
			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_block_pool_head), mask_block_pool_head_next, mask_block_pool_head) == mask_block_pool_head)
			{
				NodeFree(block_pool_head);
				break;
			}
		}

		//Replacement 적용 및 사용 갯수 추가
		_InterlockedIncrement(&_block_chunk_use_cnt);

		//어차피 데이터가 아닌 해당 node가 필요하므로 굳이 data를 복사하지 않고 해당 노드를 꺼내 사용한다.
		return blocks;
	}

	//어차피 이중 delete는 에러 대상이므로 누군가 한번만 호출한다고 고려해야 한다.

	void BlockFree(Block* p_block)
	{
		Node* block_pool_tail;
		Node* mask_block_pool_tail;

		Node* mask_alloc_node;

		//Node 할당 및 초기화
		Node* alloc_node = NodeAlloc();
		alloc_node->next_node = nullptr;
		alloc_node->block_top = p_block;

		mask_alloc_node = MaskNode(alloc_node);

		while (true)
		{
			mask_block_pool_tail = _block_pool_tail;

			block_pool_tail = UnMaskNode(mask_block_pool_tail);

			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&block_pool_tail->next_node), mask_alloc_node, nullptr) == nullptr)
			{
				InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_block_pool_tail), mask_alloc_node, mask_block_pool_tail);
				break;
			}

			InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_block_pool_tail), block_pool_tail->next_node, mask_block_pool_tail);
		}

		_InterlockedIncrement(&_block_chunk_release_cnt);
		_InterlockedDecrement(&_block_chunk_use_cnt);
	}


	Node* NodeAlloc()
	{
		Node* mask_node_pool_tail;
		Node* node_pool_tail;
		Node* mask_node_pool_head;
		Node* node_pool_head;
		Node* node_pool_head_next;
		Node* alloc_node;

		long size = _InterlockedDecrement(&_node_release_cnt);

		//release_cnt 숫자를 보고 없다면 할당헤서 나간다. 이때 해당 노드의 Chunk도 사용되므로 초기화가 필요하다.
		//next null의 경우 queue에 연결하는 경우만 필요하므로 그냥 초기화 하지 않는다.
		if (size < 0)
		{
			_InterlockedIncrement(&_node_release_cnt);

			alloc_node = (Node*)malloc(sizeof(Node));
			//Node - Chunk - ChunkNode의 생성자 연쇄 호출을 막기 위해 malloc으로 할당 뒤, 각 ChunkNode의 주소 값과 false일 경우 생성자를 호출해주는 초기화 함수이다.
			_InterlockedIncrement(&_node_use_cnt);
			_InterlockedIncrement(&_node_capacity_cnt);
			return alloc_node;
		}

		while (true)
		{
			mask_node_pool_tail = _node_pool_tail;
			node_pool_tail = UnMaskNode(mask_node_pool_tail);

			//만약 연결되어 있지 않다면 연결한다. 이때 tail->next의 경우 tail이 변경되지 않았다면 항상 동일하므로 굳이 저장하지 않는다.
			if (node_pool_tail->next_node != nullptr)
			{
				InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_node_pool_tail), node_pool_tail->next_node, mask_node_pool_tail);
				continue;
			}

			//지금은 data를 사용하는 것이 아닌 현재 노드 자체를 사용하므로 해당 노드를 반환해주면 된다.
			mask_node_pool_head = _node_pool_head;
			node_pool_head = UnMaskNode(mask_node_pool_head);

			//다른 스레드에서 제거하였거나 붕뜨는 상황이라 연결되지 않았다면 발생 가능한 상황이다.
			if (node_pool_head->next_node == nullptr)
				continue;

			//마찬가지로 head->next는 head가 변하지 않았다면 동일하므로 head->next로 변경하고 나가준다.
			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_node_pool_head), node_pool_head->next_node, mask_node_pool_head) == mask_node_pool_head)
			{
				break;
			}

		}
		//Replacement 적용 및 사용 갯수 추가
		_InterlockedIncrement(&_node_use_cnt);

		//어차피 데이터가 아닌 해당 node가 필요하므로 굳이 data를 복사하지 않고 해당 노드를 꺼내 사용한다.
		return node_pool_head;

	}

	void NodeFree(Node* free_node)
	{
		Node* node_pool_tail;
		Node* mask_node_pool_tail;

		free_node->next_node = nullptr;
		Node* mask_free_node = MaskNode(free_node);

		while (true)
		{
			mask_node_pool_tail = _node_pool_tail;

			node_pool_tail = UnMaskNode(mask_node_pool_tail);

			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&node_pool_tail->next_node), mask_free_node, nullptr) == nullptr)
			{
				InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_node_pool_tail), mask_free_node, mask_node_pool_tail);
				break;
			}

			InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_node_pool_tail), node_pool_tail->next_node, mask_node_pool_tail);
		}

		_InterlockedIncrement(&_node_release_cnt);
		_InterlockedDecrement(&_node_use_cnt);
	}

	__forceinline Node* MaskNode(Node* node)
	{
		uint64 cmpBit = _InterlockedIncrement(reinterpret_cast<unsigned long long*>(&_cmp_bit));
		cmpBit = cmpBit << LIST_BITMASK_SHIFT47;
		return reinterpret_cast<Node*>((reinterpret_cast<uint64>(node) | cmpBit));
	}

	__forceinline Node* UnMaskNode(Node* node)
	{
		return 	reinterpret_cast<Node*>(reinterpret_cast<uint64>(node) & LIST_MASK_POINTER64);
	}


private:
	HANDLE _block_pool_manager;
	alignas(64) uint64 _cmp_bit;

	alignas(64) Node* _block_pool_head;
	Node* _block_pool_tail;

	alignas(64) Node* _node_pool_head;
	Node* _node_pool_tail;

	alignas(64) long _node_capacity_cnt;
	long _node_use_cnt;
	long _node_release_cnt;

	alignas(64) long _block_chunk_capacity_cnt;
	long _block_chunk_use_cnt;
	long _block_chunk_release_cnt;

	alignas(64) long _block_use_cnt;
	long _block_release_cnt;

	static thread_local BlockHeader _tls_alloc_block_header;
	static thread_local BlockHeader _tls_free_block_header;
};

template<typename DATA, bool Replacement>
thread_local typename LockFreeObjectPoolTLSList<DATA, Replacement>::BlockHeader LockFreeObjectPoolTLSList<DATA, Replacement>::_tls_alloc_block_header;

template<typename DATA, bool Replacement>
thread_local typename LockFreeObjectPoolTLSList<DATA, Replacement>::BlockHeader LockFreeObjectPoolTLSList<DATA, Replacement>::_tls_free_block_header;

#pragma warning(default: 4101)
