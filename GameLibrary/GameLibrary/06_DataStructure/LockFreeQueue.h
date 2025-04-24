#pragma once
#include "LockFreeObjectPool.h"
#include "LockFreeObjectPoolTLS.h"

template<typename T, int MAX_QUEUE_SIZE = df_QUEUE_SIZE>
class LockFreeQueue
{
	const uint64 SHIFT_MASK47 = 47;
	const uint64 MEMORY_MASK = ((uint64)1 << 47) - 1;

	template<typename T>
	struct Node
	{
		Node()
		{

		}

		Node(T&& data, Node<T>* next) : data(std::forward<T>(data)), next(next)
		{

		}
		T data;
		Node<T>* next;
	};

public:

	LockFreeQueue() : node_pool(0), _size(0), _cmp_bit(0)
	{
		_tail = node_pool.Alloc();
		_tail->next = nullptr;
		_tail = MaskNode(_tail);
		_head = _tail;
	}

	//삭제가 이루어지는 것은 모두 처리 후??
	~LockFreeQueue()
	{
		Node<T>* head = UnMaskNode(_head);
		while (head->next != nullptr)
		{
			_head = head->next;
			node_pool.Free(head);
			head = UnMaskNode(_head);
		}

	}

	long Enqueue(T data)
	{
		if (InterlockedIncrement(&_max_cnt) >= MAX_QUEUE_SIZE)
		{
			InterlockedDecrement(&_max_cnt);
			return MAX_QUEUE_SIZE;
		}

		Node<T>* new_node = node_pool.Alloc(std::move(data), nullptr);
		Node<T>* mask_new_node;
		Node<T>* tail;
		Node<T>* mask_tail;

		mask_new_node = MaskNode(new_node);

		while (true)
		{
			mask_tail = _tail;
			tail = UnMaskNode(mask_tail);

			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&tail->next), mask_new_node, nullptr) == nullptr)
			{
				InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_tail), mask_new_node, mask_tail);
				break;
			}

			InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_tail), tail->next, mask_tail);
		}

		return 	_InterlockedIncrement(&_size);
	}

	bool Dequeue(T& data)
	{
		long size = _InterlockedDecrement(&_size);

		if (size < 0)
		{
			size = _InterlockedIncrement(&_size);
			return false;
		}

		Node<T>* mask_tail;
		Node<T>* tail;

		while (true)
		{
			mask_tail = _tail;
			tail = UnMaskNode(mask_tail);

			if (tail->next != nullptr)
			{
				InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_tail), tail->next, mask_tail);
				continue;
			}

			Node<T>* mask_head = _head;
			Node<T>* head = UnMaskNode(mask_head);

			Node<T>* mask_head_next = head->next;
			Node<T>* head_next = UnMaskNode(mask_head_next);

			if (mask_head_next == nullptr)
				continue;

			data = head_next->data;

			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_head), mask_head_next, mask_head) == mask_head)
			{
				node_pool.Free(head);
				break;
			}
		}

		InterlockedDecrement(&_max_cnt);

		return true;
	}

	long Size()
	{
		return _size;
	}

	long* SizePtr()
	{
		return &_size;
	}

	long MaxQueueSize()
	{
		return MAX_QUEUE_SIZE;
	}

private:

	Node<T>* MaskNode(Node<T>* mem_node)
	{
		uint64 cmp = _InterlockedIncrement(&_cmp_bit) << SHIFT_MASK47;
		return reinterpret_cast<Node<T>*>(reinterpret_cast<size_t>(mem_node) | cmp);
	}

	Node<T>* UnMaskNode(Node<T>* mask_node)
	{
		return reinterpret_cast<Node<T>*>(reinterpret_cast<size_t>(mask_node) & MEMORY_MASK);
	}

private:
	long _size;
	long _max_cnt;
	uint64 _cmp_bit;
	Node<T>* _head;
	Node<T>* _tail;
	LockFreeObjectPool<Node<T>, true> node_pool;
};




//Lock Free Queue : 공용 풀로 가는 이유, 너무 size가 커진다.
//Lock Free Queue : 공용풀로 가면 속도가 빨라진다.


