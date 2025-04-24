#pragma once

#ifdef __POOL_DEBUG__
#define g_poolLogLevel 1
#else 
#define g_poolLogLevel 2
#endif // __POOL_DEBUG__

//Replacement True에 대한 처리

#if g_poolLogLevel > 1

template <typename DATA, bool tReplacementNew>
class LockFreeObjectPool
{
#pragma warning(disable: 6011)

	static const uint64	COOKIE_ERASE = 0xeeee'eeee'eeee'eeee;
	static const uint64 BITMASK_SHIFT47 = 47;
	static const uint64 MASK_POINTER64 = (((uint64)1 << 47) - 1);

	struct st_NODE
	{
		st_NODE* nextNode;
		DATA data;
	};

public:

	template<typename... Args>
	LockFreeObjectPool(int iBlockNum = 0, Args&&... args) : _iCapacity(0), _iUseCount(0), _cmpBit(0), _releaseCount(0)
	{
		_pFreeNode = nullptr;

		for (int cnt = 0; cnt < iBlockNum; cnt++)
		{
			st_NODE* newNode = (st_NODE*)malloc(sizeof(st_NODE));
			newNode->nextNode = _pFreeNode;

			if constexpr (tReplacementNew == false)
				new(&newNode->data) DATA(std::forward<Args>(args)...);

			newNode = MaskNode(newNode);
			_pFreeNode = newNode;
		}

		_iCapacity = iBlockNum;
	}

	~LockFreeObjectPool()
	{
		st_NODE* tempNode;

		while (_pFreeNode != nullptr)
		{
			tempNode = UnMaskNode(_pFreeNode);
			_pFreeNode = tempNode->nextNode;

			if constexpr (tReplacementNew == false)
				tempNode->data.~DATA();

			free(tempNode);
		}
	}

	template<typename... Args>
	DATA* Alloc(Args&&... args)
	{
		st_NODE* outNode;
		st_NODE* cmpNode;

		while (true)
		{
			cmpNode = _pFreeNode;

			if (cmpNode == nullptr)
			{
				cmpNode = (st_NODE*)malloc(sizeof(st_NODE));
				new(&cmpNode->data) DATA(std::forward<Args>(args)...);

				AtomicIncrement32(&_iCapacity);
				AtomicIncrement32(&_iUseCount);
				return  &cmpNode->data;
			}

			outNode = UnMaskNode(cmpNode);

			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_pFreeNode), outNode->nextNode, cmpNode) == cmpNode)
			{
				break;
			}
		}

		//Replacement 적용 및 사용 갯수 추가
		if constexpr (tReplacementNew)
			new(&outNode->data) DATA(std::forward<Args>(args)...);

		AtomicIncrement32(&_iUseCount);
		AtomicDecrement32(&_releaseCount);

		return &outNode->data;
	}

	//어차피 이중 delete는 에러 대상이므로 누군가 한번만 호출한다고 고려해야 한다.
	void	Free(DATA* pData)
	{
		if constexpr (tReplacementNew)
			pData->~DATA();

		st_NODE* freeNode;
		st_NODE* tempNode = BringOrignNode(pData);

		freeNode = MaskNode(tempNode);

		while (true)
		{
			st_NODE* top = _pFreeNode;
			tempNode->nextNode = top;

			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_pFreeNode), freeNode, top) == top)
			{
				break;
			}
		}

		AtomicDecrement32(&_iUseCount);
		AtomicIncrement32(&_releaseCount);

	}

	uint32		GetCapacity(void) { return _iCapacity; }
	uint32		GetUseCount(void) { return _iUseCount; }
	uint32		GetReleaseCount(void) { return _releaseCount; }

private:

	st_NODE* MaskNode(st_NODE* node)
	{
		uint64 cmpBit = AtomicIncrement64(&_cmpBit);
		cmpBit = cmpBit << BITMASK_SHIFT47;
		return reinterpret_cast<st_NODE*>((reinterpret_cast<uint64>(node) | cmpBit));
	}

	st_NODE* UnMaskNode(st_NODE* node)
	{
		return 	reinterpret_cast<st_NODE*>(reinterpret_cast<uint64>(node) & MASK_POINTER64);
	}

	st_NODE* BringOrignNode(DATA* pData)
	{
		return reinterpret_cast<st_NODE*>(reinterpret_cast<size_t>(pData) - offsetof(st_NODE, data));
	}

	// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.
private:
	uint64 _cmpBit;
	st_NODE* _pFreeNode;

	alignas(64) uint32 _iCapacity;
	alignas(64) uint32 _iUseCount;
	alignas(64) uint32 _releaseCount;
};



#else
template <typename DATA, bool tReplacementNew>
class LockFreeObjectPool
{

	static const uint64	COOKIE_ERASE = 0xeeee'eeee'eeee'eeee;
	static const uint64 BITMASK_SHIFT47 = 47;
	static const uint64 MASK_POINTER64 = (((uint64)1 << 47) - 1);

	struct st_NODE
	{
		st_NODE() {}

		__forceinline void InitAllocInfo(uint64 cookie)
		{
			cookieFront = cookie;
			cookieBack = cookie;
		}

		__forceinline void InitReleaseInfo()
		{
			cookieFront = COOKIE_ERASE;
			cookieBack = COOKIE_ERASE;
		}

		st_NODE* nextNode;
		uint64 cookieFront;
		DATA data;
		uint64 cookieBack;
	};

public:
	template<typename... Args>
	LockFreeObjectPool(int iBlockNum = 0, Args&&... args) : _cookieInsert(reinterpret_cast<uint64>(this)) _iCapacity(0), _iUseCount(0), _cmpBit(0)
	{
		_pFreeNode = nullptr;

		for (int cnt = 0; cnt < iBlockNum; cnt++)
		{
			//노드 생성
			st_NODE* newNode = (st_NODE*)malloc(sizeof(st_NODE));
			//cookie 삽입
			newNode->InitReleaseInfo();
			newNode->nextNode = _pFreeNode;

			if constexpr (tReplacementNew == false)
				new(&newNode->data) DATA(std::forward<Args>(args)...);

			//ABA Problem 해결을 위한 bitcnt 적용
			newNode = MaskNode(newNode);
			_pFreeNode = newNode;
		}
		_iCapacity = iBlockNum;
	}

	virtual	~LockFreeObjectPool()
	{
		if (_iUseCount != 0)
			DebugBreak();

		while (_pFreeNode != nullptr)
		{
			st_NODE* tempNode = _pFreeNode;
			tempNode = UnMaskNode(tempNode);
			_pFreeNode = tempNode->nextNode;

			if constexpr (tReplacementNew == false)
				tempNode->data.~DATA();

			free(tempNode);
		}

	}

	template<typename... Args>
	DATA* Alloc(Args&&... args)
	{
		st_NODE* outNode;
		st_NODE* newTop;
		st_NODE* cmpNode;

		//없을 경우 생성 -> Dynamic Heap이 아니므로 직렬화가 이루어진다.
		if (_pFreeNode == nullptr)
		{
			outNode = (st_NODE*)malloc(sizeof(st_NODE));
			new(&outNode->data) DATA(std::forward<Args>(args)...);
			outNode->InitAllocInfo(_cookieInsert);

			//Cnt 증가
			AtomicIncrement32(&_iCapacity);
			AtomicIncrement32(&_iUseCount);
			return  &outNode->data;
		}

		while (true)
		{
			cmpNode = _pFreeNode;
			outNode = UnMaskNode(cmpNode);
			newTop = outNode->nextNode;

			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_pFreeNode), newTop, cmpNode) == cmpNode)
			{
				break;
			}
		}

		//Cookie 적용
		outNode->InitAllocInfo(_cookieInsert);

		//Replacement 적용
		if constexpr (tReplacementNew)
			new(&outNode->data) DATA(std::forward<Args>(args)...);

		//사용 갯수 추가
		AtomicIncrement32(&_iUseCount);

		return &outNode->data;
	}

	void	Free(DATA* pData)
	{
		st_NODE* tempNode = BringOrignNode(pData);
		st_NODE* freeNode;

		if (CheckDebugInfo(tempNode) == false)
			__debugbreak();

		//소멸자 호출
		if constexpr (tReplacementNew)
			tempNode->data.~DATA();

		//삭제 쿠키 기입
		tempNode->InitReleaseInfo();

		freeNode = MaskNode(tempNode);

		while (true)
		{
			st_NODE* top = _pFreeNode;
			tempNode->nextNode = top;

			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_pFreeNode), freeNode, top) == top)
			{
				break;
			}
		}

		AtomicDecrement32(&_iUseCount);
	}


	uint32		GetCapacityCount(void) { return _iCapacity; }
	uint32		GetUseCount(void) { return _iUseCount; }

private:

	st_NODE* MaskNode(st_NODE* node)
	{
		uint64 cmpBit = AtomicIncrement64(&_cmpBit);
		cmpBit = cmpBit << BITMASK_SHIFT47;
		return reinterpret_cast<st_NODE*>((reinterpret_cast<uint64>(node) | cmpBit));
	}

	st_NODE* UnMaskNode(st_NODE* node)
	{
		return 	reinterpret_cast<st_NODE*>(reinterpret_cast<uint64>(node) & MASK_POINTER64);
	}

	st_NODE* BringOrignNode(DATA* pData)
	{
		return reinterpret_cast<st_NODE*>(reinterpret_cast<size_t>(pData) - offsetof(st_NODE, data));
	}

	bool CheckDebugInfo(st_NODE* node)
	{

		if (node->cookieFront != _cookieInsert || node->cookieBack != _cookieInsert)
		{
			if (node->cookieFront == _cookieErase || node->cookieBack == _cookieErase)
			{
				std::wcout << L"Memory Double Free " << &node->data << '\n';
			}
			else
			{
				std::wcout << L"Cookie corruption " << &node->data << '\n';
			}
			return false;
		}
		return true;
	}

	// 스택 방식으로 반환된 (미사용) 오브젝트 블럭을 관리.
private:
	uint32 _iCapacity;
	uint32 _iUseCount;
	uint64 _cmpBit;
	st_NODE* _pFreeNode;
	unsigned int _cookieInsert;
};

#endif


