#pragma once

/*---------------------------------------------------
-----------------------------------------------------
#			LockFreeObjectPoolTLS (Release)
-----------------------------------------------------
----------------------------------------------------*/


//Node - Chunk - ChunkNode ������ ������ �̷������ ChunkNode�� ��� �迭�� �����͸� ������ �ִ�.
//Node�� ��� �ϳ��� ����Ʈ�� ������ �Ǹ� ABA problem�� ���ϱ� ���� ����ŷ ó���� �ϰ� �ִ�.
//Chunk�� ��� ChunkNode�� ������ ȣ���� ���� ���� malloc�� ���� �Ҵ��� �̷������ �ʱ�ȭ�� ��� ���ڿ� ChunkNode ������ ȣ���� ChunkInitr �Լ��� ���� ���ش�.
//ChunkNode�� ��� Replacement�� ���� �ʱ� ������ ȣ���� �����Ǿ�����.
//���������� �Ҹ�ÿ��� Replacement�� false�� ��쿡 ���ؼ��� Chunk �Ҹ��� ȣ���� �̷������ �ƴ� ��� �׻� ChunkNode �Ҹ��ڰ� ȣ��ȴ�.
//�ʱ� Chunk�� ���� ���ڷ� �Ѱ��� �� �ְ� MAX_CHUNK_SIZE�� ���ؼ� CHUNK ũ�⸦ �����ϴ� ���� �����ϴ�.


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
//		//�Ҹ��ڰ� ȣ��Ǵ��� Ȯ������
//		Chunk() = default;
//		~Chunk() = default;
//
//		template<typename... Args>
//		__forceinline DATA* ChunkNodeAlloc(Args&&... args)
//		{
//			//interlock�� �ƴϿ��� ��������??
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
//		//������ Chunk�� ChunkNode ������ 
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
//			//false�� ��츸 �Ҹ��ڸ� ���� ChunkNode�� �Ҹ��ڸ� ȣ�����ش�.
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
//			//Replacement�� false��� �ѹ��� �ʱ�ȭ�̹Ƿ� ���� �Ҵ�ÿ� �ʱ�ȭ�� �̷������.
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
//		//true�� ��츸 ���ڸ� �Ѱ� �ʱ�ȭ�� ����ȴ�.
//		if (Replacement)
//			data = tls_chunk->ChunkNodeAlloc(std::forward<Args>(args)...);
//		else
//			data = tls_chunk->ChunkNodeAlloc();
//
//
//		//�� ���� nullptr�� �ʱ�ȭ �̶� �̱��̹Ƿ� ������ ���� �ʴ´�.
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
//		//������ ���� ������ ���� Chunk�� ���� ó���� �� ������ �Ʒ��� �ڵ尡 ó������ �����Ƿ� ������ ���� �ʴ´�.
//		if (chunk_node->_chunk->ChunkNodeFree(chunk_node) == MAX_CHUNK_SIZE)
//		{
//			InterlockedAdd(reinterpret_cast<long*>(&_chunknode_release_cnt), MAX_CHUNK_SIZE * -1);
//			ChunkFree(chunk_node->_chunk);
//		}
//		//Chunknode ��ü�� �ǵ帮�� ���� �ƴϹǷ� �Ʒ����� ó���ص� ��� X
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
//			//���� Top Node Ȯ��
//			cmpNode = _top_free;
//
//			//NULL�̸� �����Ƿ� �׳� �� ��忡 �Ҵ� �޾� ��ȯ
//			if (cmpNode == nullptr)
//			{
//				cmpNode = (st_NODE*)malloc(sizeof(st_NODE));
//
//				//Node - Chunk - ChunkNode�� ������ ���� ȣ���� ���� ���� malloc���� �Ҵ� ��, �� ChunkNode�� �ּ� ���� false�� ��� �����ڸ� ȣ�����ִ� �ʱ�ȭ �Լ��̴�.
//				cmpNode->_chunk.ChunkInit(std::forward<Args>(args)...);
//				InterlockedIncrement(&_chunk_use_cnt);
//				InterlockedIncrement(&_chunk_capacity);
//				return  &cmpNode->_chunk;
//			}
//
//			//�����Ƿ� ���� �ּҷ� ����, CMP �� �ּҸ� �ξ�� ��
//			outNode = UnMaskNode(cmpNode);
//
//			//������ outNode�� nextNode�� Memory Pool�̹Ƿ� ������� �ʴ´�. ���� �׳� �ٷ� ���
//			//���� �� unique�� �ּҰ� �����ϴٸ� (������ ���) -> �� �� break �ش� ���� ���� ���� �ǵ帱 �� �����Ƿ� data�� ��ȯ���ش�.
//			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_top_free), outNode->nextNode, cmpNode) == cmpNode)
//			{
//				break;
//			}
//		}
//
//		//Replacement ���� �� ��� ���� �߰�
//		InterlockedIncrement(&_chunk_use_cnt);
//		InterlockedDecrement(&_chunk_release_cnt);
//
//		return &outNode->_chunk;
//	}
//
//	//������ ���� delete�� ���� ����̹Ƿ� ������ �ѹ��� ȣ���Ѵٰ� ����ؾ� �Ѵ�.
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
//			//tempNode->next�� �ٷ� ������� ���ϴ� ������ interlocked �ϴ� ���� ������ next�� �޶��� �� �����Ƿ� �񱳽ÿ��� �������� �ʿ��ϴ�.
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
		//�Ҹ��ڰ� ȣ��Ǵ��� Ȯ������
		Chunk() = default;
		~Chunk() = default;

		template<typename... Args>
		__forceinline DATA* ChunkNodeAlloc(Args&&... args)
		{
			//interlock�� �ƴϿ��� ��������??
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

		//Replacement true�̸� ������ ���� X, false��� �Ѱ��־�� �Ѵ�.
		temp_node->_chunk.ChunkInit(std::forward<Args>(args)...);
		//������ Chunk�� ChunkNode ������ 
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
			//false�� ��츸 �Ҹ��ڸ� ���� ChunkNode�� �Ҹ��ڸ� ȣ�����ش�.
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
			//Replacement�� false��� �ѹ��� �ʱ�ȭ�̹Ƿ� ���� �Ҵ�ÿ� �ʱ�ȭ�� �̷������.
			if constexpr (Replacement == false)
				_tls_chunk = ChunkAlloc(std::forward<Args>(args)...);
			else
				_tls_chunk = ChunkAlloc();

			_tls_chunk->ChunkClear();
			InterlockedAdd(reinterpret_cast<long*>(&_chunknode_release_cnt), MAX_CHUNK_SIZE);
		}

		Chunk* tls_chunk = _tls_chunk;

		DATA* data;

		//true�� ��츸 ���ڸ� �Ѱ� �ʱ�ȭ�� ����ȴ�.
		if constexpr (Replacement)
			data = tls_chunk->ChunkNodeAlloc(std::forward<Args>(args)...);
		else
			data = tls_chunk->ChunkNodeAlloc();


		//�� ���� nullptr�� �ʱ�ȭ �̶� �̱��̹Ƿ� ������ ���� �ʴ´�.
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

		//release_cnt ���ڸ� ���� ���ٸ� �Ҵ��켭 ������. �̶� �ش� ����� Chunk�� ���ǹǷ� �ʱ�ȭ�� �ʿ��ϴ�.
		//next null�� ��� queue�� �����ϴ� ��츸 �ʿ��ϹǷ� �׳� �ʱ�ȭ ���� �ʴ´�.
		if (size < 0)
		{
			_InterlockedIncrement(&_chunk_release_cnt);
			head = (st_NODE*)malloc(sizeof(st_NODE));
			//Node - Chunk - ChunkNode�� ������ ���� ȣ���� ���� ���� malloc���� �Ҵ� ��, �� ChunkNode�� �ּ� ���� false�� ��� �����ڸ� ȣ�����ִ� �ʱ�ȭ �Լ��̴�.
			head->_chunk.ChunkInit(std::forward<Args>(args)...);
			InterlockedIncrement(&_chunk_use_cnt);
			InterlockedIncrement(&_chunk_capacity);
			return  &head->_chunk;
		}

		while (true)
		{
			mask_tail = _tail;
			tail = UnMaskNode(mask_tail);

			//���� ����Ǿ� ���� �ʴٸ� �����Ѵ�. �̶� tail->next�� ��� tail�� ������� �ʾҴٸ� �׻� �����ϹǷ� ���� �������� �ʴ´�.
			if (tail->nextNode != nullptr)
			{
				InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_tail), tail->nextNode, mask_tail);
				continue;
			}


			//������ data�� ����ϴ� ���� �ƴ� ���� ��� ��ü�� ����ϹǷ� �ش� ��带 ��ȯ���ָ� �ȴ�.
			mask_head = _head;
			head = UnMaskNode(mask_head);

			//�ٸ� �����忡�� �����Ͽ��ų� �ضߴ� ��Ȳ�̶� ������� �ʾҴٸ� �߻� ������ ��Ȳ�̴�.
			if (head->nextNode == nullptr)
				continue;

			//���������� head->next�� head�� ������ �ʾҴٸ� �����ϹǷ� head->next�� �����ϰ� �����ش�.
			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_head), head->nextNode, mask_head) == mask_head)
			{
				break;
			}

		}
		//Replacement ���� �� ��� ���� �߰�
		InterlockedIncrement(&_chunk_use_cnt);

		//������ �����Ͱ� �ƴ� �ش� node�� �ʿ��ϹǷ� ���� data�� �������� �ʰ� �ش� ��带 ���� ����Ѵ�.
		return &head->_chunk;
	}

	//������ ���� delete�� ���� ����̹Ƿ� ������ �ѹ��� ȣ���Ѵٰ� ����ؾ� �Ѵ�.
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

