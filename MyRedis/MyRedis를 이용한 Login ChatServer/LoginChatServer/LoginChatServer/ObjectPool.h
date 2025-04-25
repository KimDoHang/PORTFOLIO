#pragma once

#ifdef __POOL_DEBUG__
#define g_poolLogLevel 1
#else 
#define g_poolLogLevel 2
#endif // __POOL_DEBUG__

//Replacement True에 대한 처리



#if g_poolLogLevel > 1

template <typename DATA, bool tReplacementNew>
class ObjectPool
{
#pragma warning(disable: 6011)

	struct st_NODE
	{
		st_NODE* nextNode;
		DATA data;
	};

public:

	template<typename... Args>
	ObjectPool(int iBlockNum = 0, Args&&... args) : _iCapacity(0), _iUseCount(0), _cmpBit(0), _releaseCount(0)
	{
		_pFreeNode = nullptr;

		for (int cnt = 0; cnt < iBlockNum; cnt++)
		{
			st_NODE* newNode = (st_NODE*)malloc(sizeof(st_NODE));
			newNode->nextNode = _pFreeNode;

			if constexpr (tReplacementNew == false)
				new(&newNode->data) DATA(std::forward<Args>(args)...);

			_pFreeNode = newNode;
		}

		_iCapacity = iBlockNum;
	}

	virtual	~ObjectPool()
	{
		st_NODE* tempNode;

		while (_pFreeNode != nullptr)
		{
			tempNode = _pFreeNode;
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

		if (_pFreeNode == nullptr)
		{
			outNode = (st_NODE*)malloc(sizeof(st_NODE));
			new(&outNode->data) DATA(std::forward<Args>(args)...);

			_iCapacity++;
			_iUseCount++;
			return  &outNode->data;
		}

		outNode = _pFreeNode;
		_pFreeNode = _pFreeNode->nextNode;

		if constexpr (tReplacementNew)
			new(&outNode->data) DATA(std::forward<Args>(args)...);

		_iUseCount++;
		_releaseCount--;
		return &outNode->data;
	}


	//어차피 이중 delete는 에러 대상이므로 누군가 한번만 호출한다고 고려해야 한다.
	void	Free(DATA* pData)
	{
		if constexpr (tReplacementNew)
			pData->~DATA();

		st_NODE* freeNode = reinterpret_cast<st_NODE*>(reinterpret_cast<size_t>(pData) - offsetof(st_NODE, data));

		freeNode->nextNode = _pFreeNode;
		_pFreeNode = freeNode;

		_iUseCount--;
		_releaseCount++;
	}

	uint32		GetCapacity(void) { return _iCapacity; }
	uint32		GetUseCount(void) { return _iUseCount; }
	uint32		GetReleaseCount(void) { return _releaseCount; }

private:


private:
	uint64 _cmpBit;
	st_NODE* _pFreeNode;

	uint32 _iCapacity;
	uint32 _iUseCount;
	uint32 _releaseCount;
};



#else


#endif


