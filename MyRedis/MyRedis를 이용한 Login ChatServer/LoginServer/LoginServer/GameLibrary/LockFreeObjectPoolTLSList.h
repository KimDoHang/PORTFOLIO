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

			//false�� ��츸 �Ҹ��ڸ� ���� ChunkNode�� �Ҹ��ڸ� ȣ�����ش�.
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

			//false�� ��츸 �Ҹ��ڸ� ���� ChunkNode�� �Ҹ��ڸ� ȣ�����ش�.

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

		//BlockLid�� block�� �Ҵ� �ޱ�
		if (alloc_header->top == nullptr)
		{
			//Replacement�� false��� �ѹ��� �ʱ�ȭ�̹Ƿ� ���� �Ҵ�ÿ� �ʱ�ȭ�� �̷������.
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

		//Block �ű��
		Block* block = alloc_header->top;
		alloc_header->top = alloc_header->top->next_block;

		//true�� ��츸 ���ڸ� �Ѱ� �ʱ�ȭ�� ����ȴ�.
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

		//release_cnt ���ڸ� ���� ���ٸ� �Ҵ��켭 ������. �̶� �ش� ����� Chunk�� ���ǹǷ� �ʱ�ȭ�� �ʿ��ϴ�.
		//next null�� ��� queue�� �����ϴ� ��츸 �ʿ��ϹǷ� �׳� �ʱ�ȭ ���� �ʴ´�.
		if (size < 0)
		{
			_InterlockedIncrement(&_block_chunk_release_cnt);

			blocks = BlockChunkInit(std::forward<Args>(args)...);
			//Node - Chunk - ChunkNode�� ������ ���� ȣ���� ���� ���� malloc���� �Ҵ� ��, �� ChunkNode�� �ּ� ���� false�� ��� �����ڸ� ȣ�����ִ� �ʱ�ȭ �Լ��̴�.
			_InterlockedIncrement(&_block_chunk_use_cnt);
			_InterlockedIncrement(&_block_chunk_capacity_cnt);
			return blocks;
		}

		while (true)
		{
			mask_block_pool_tail = _block_pool_tail;
			block_pool_tail = UnMaskNode(mask_block_pool_tail);

			//���� ����Ǿ� ���� �ʴٸ� �����Ѵ�. �̶� tail->next�� ��� tail�� ������� �ʾҴٸ� �׻� �����ϹǷ� ���� �������� �ʴ´�.
			if (block_pool_tail->next_node != nullptr)
			{
				InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_block_pool_tail), block_pool_tail->next_node, mask_block_pool_tail);
				continue;
			}

			//������ data�� ����ϴ� ���� �ƴ� ���� ��� ��ü�� ����ϹǷ� �ش� ��带 ��ȯ���ָ� �ȴ�.
			mask_block_pool_head = _block_pool_head;
			block_pool_head = UnMaskNode(mask_block_pool_head);

			mask_block_pool_head_next = block_pool_head->next_node;
			block_pool_head_next = UnMaskNode(mask_block_pool_head_next);

			//�ٸ� �����忡�� �����Ͽ��ų� �ضߴ� ��Ȳ�̶� ������� �ʾҴٸ� �߻� ������ ��Ȳ�̴�.
			if (block_pool_head_next == nullptr)
				continue;

			blocks = block_pool_head_next->block_top;

			//���������� head->next�� head�� ������ �ʾҴٸ� �����ϹǷ� head->next�� �����ϰ� �����ش�.
			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_block_pool_head), mask_block_pool_head_next, mask_block_pool_head) == mask_block_pool_head)
			{
				NodeFree(block_pool_head);
				break;
			}
		}

		//Replacement ���� �� ��� ���� �߰�
		_InterlockedIncrement(&_block_chunk_use_cnt);

		//������ �����Ͱ� �ƴ� �ش� node�� �ʿ��ϹǷ� ���� data�� �������� �ʰ� �ش� ��带 ���� ����Ѵ�.
		return blocks;
	}

	//������ ���� delete�� ���� ����̹Ƿ� ������ �ѹ��� ȣ���Ѵٰ� ����ؾ� �Ѵ�.

	void BlockFree(Block* p_block)
	{
		Node* block_pool_tail;
		Node* mask_block_pool_tail;

		Node* mask_alloc_node;

		//Node �Ҵ� �� �ʱ�ȭ
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

		//release_cnt ���ڸ� ���� ���ٸ� �Ҵ��켭 ������. �̶� �ش� ����� Chunk�� ���ǹǷ� �ʱ�ȭ�� �ʿ��ϴ�.
		//next null�� ��� queue�� �����ϴ� ��츸 �ʿ��ϹǷ� �׳� �ʱ�ȭ ���� �ʴ´�.
		if (size < 0)
		{
			_InterlockedIncrement(&_node_release_cnt);

			alloc_node = (Node*)malloc(sizeof(Node));
			//Node - Chunk - ChunkNode�� ������ ���� ȣ���� ���� ���� malloc���� �Ҵ� ��, �� ChunkNode�� �ּ� ���� false�� ��� �����ڸ� ȣ�����ִ� �ʱ�ȭ �Լ��̴�.
			_InterlockedIncrement(&_node_use_cnt);
			_InterlockedIncrement(&_node_capacity_cnt);
			return alloc_node;
		}

		while (true)
		{
			mask_node_pool_tail = _node_pool_tail;
			node_pool_tail = UnMaskNode(mask_node_pool_tail);

			//���� ����Ǿ� ���� �ʴٸ� �����Ѵ�. �̶� tail->next�� ��� tail�� ������� �ʾҴٸ� �׻� �����ϹǷ� ���� �������� �ʴ´�.
			if (node_pool_tail->next_node != nullptr)
			{
				InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_node_pool_tail), node_pool_tail->next_node, mask_node_pool_tail);
				continue;
			}

			//������ data�� ����ϴ� ���� �ƴ� ���� ��� ��ü�� ����ϹǷ� �ش� ��带 ��ȯ���ָ� �ȴ�.
			mask_node_pool_head = _node_pool_head;
			node_pool_head = UnMaskNode(mask_node_pool_head);

			//�ٸ� �����忡�� �����Ͽ��ų� �ضߴ� ��Ȳ�̶� ������� �ʾҴٸ� �߻� ������ ��Ȳ�̴�.
			if (node_pool_head->next_node == nullptr)
				continue;

			//���������� head->next�� head�� ������ �ʾҴٸ� �����ϹǷ� head->next�� �����ϰ� �����ش�.
			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_node_pool_head), node_pool_head->next_node, mask_node_pool_head) == mask_node_pool_head)
			{
				break;
			}

		}
		//Replacement ���� �� ��� ���� �߰�
		_InterlockedIncrement(&_node_use_cnt);

		//������ �����Ͱ� �ƴ� �ش� node�� �ʿ��ϹǷ� ���� data�� �������� �ʰ� �ش� ��带 ���� ����Ѵ�.
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
