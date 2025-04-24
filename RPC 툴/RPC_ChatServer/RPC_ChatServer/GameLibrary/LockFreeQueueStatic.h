#pragma once
#include "LockFreeObjectPoolTLS.h"

#pragma warning(disable: 26495)

template<typename T, int MAX_QUEUE_SIZE = df_QUEUE_SIZE>
class LockFreeQueueStatic
{
	static long g_queue_descriptor;
	const uint64 SHIFT_MASK47 = 47;
	const uint64 MEMORY_MASK = ((uint64)1 << 47) - 1;
	//const uint32 MAX_QUEUE_SIZE = 1000;
	struct Node
	{
		Node()
		{

		}
		Node(Node* next) : next(next)
		{

		}

		Node(T&& data, Node* next) : data(std::forward<T>(data)), next(next)
		{

		}
		T data;
		Node* next;
	};

public:

	LockFreeQueueStatic() : _size(0), _cmp_bit(0), _max_cnt(0)
	{
		_queue_descriptor = static_cast<uint64>(InterlockedIncrement(&g_queue_descriptor) - 1) << SHIFT_MASK47;
		_tail = node_pool.Alloc(reinterpret_cast<Node*>(_queue_descriptor));
		_tail = MaskNode(_tail);
		_head = _tail;
	}

	//������ �̷������ ���� ��� ó�� ��??
	~LockFreeQueueStatic()
	{
		Node* temp_node;

		while (true)
		{
			temp_node = UnMaskNode(_head);

			if (temp_node->next == nullptr)
				break;

			_head = temp_node->next;
			free(temp_node);
		}

		free(temp_node);
	}

	long Enqueue(T data)
	{
		if (InterlockedIncrement(&_max_cnt) >= MAX_QUEUE_SIZE)
		{
			InterlockedDecrement(&_max_cnt);
			return MAX_QUEUE_SIZE;
		}

		Node* new_node = node_pool.Alloc(std::move(data), reinterpret_cast<Node*>(_queue_descriptor));
		Node* mask_new_node;
		Node* tail;
		Node* mask_tail;

		mask_new_node = MaskNode(new_node);

		while (true)
		{
			mask_tail = _tail;
			tail = UnMaskNode(mask_tail);


			//tail->next�� ���� ������ ���� �ʴ� ����
			//�ش� ������ Enqueue ���� �ٸ� ������ �ش� node�� Dequeue�Ǿ� ����� �� ������ �ȴ�.
			//������ ���� Dequeue�� �Ǳ� ���ؼ��� �ݵ�� head->next != nullptr�̹Ƿ� �ݵ�� next�� �ٸ� ��尡 ���;߸� �Ѵ�.
			//���� ���Ҵ� �Ǵ� ���� �ݵ��  queue_id�� �ƴ� �ٸ� ����� �ּҷ� �ʱ�ȭ�� �̷������.
			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&tail->next), mask_new_node, reinterpret_cast<PVOID>(_queue_descriptor)) == reinterpret_cast<PVOID>(_queue_descriptor))
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

		Node* mask_tail;
		Node* tail;

		while (true)
		{
			mask_tail = _tail;
			tail = UnMaskNode(mask_tail);

			//tail->next�� ��� queueID�� ���� �ٸ��� nullptr�̶�� ���� ������ �����Ƿ� ����ŷ�� �ؼ� ó�����ش�.
			if (UnMaskNode(tail->next) != nullptr)
			{
				InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_tail), tail->next, mask_tail);
				continue;
			}

			Node* mask_head = _head;
			Node* head = UnMaskNode(mask_head);

			//����? head->next�� �ص� ���� ������?
			Node* mask_head_next = head->next;

			//Queue Descriptor�� �����Ѵ�. -> queueid�� ���� �ٸ��� nullptr�̸� continue�� �ؾ��ϹǷ�
			Node* head_next = UnMaskNode(mask_head_next);

			if (head_next == nullptr)
				continue;

			//�����͸� ���� �����̶� ���� �Ҹ��ڰ� ȣ��� �� �ֵ��� �������� �ʾҴ�. Copy ����� �ٿ���.
			data = head_next->data;

			if (InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_head), mask_head_next, mask_head) == mask_head)
			{
				node_pool.Free(head);
				break;
			}
		}

		//�ִ񰪿� ��������� ������ disconnect �ؾ��ϹǷ� �ʰ� �Ͽ� �ִ��� ���� ����� ���� �̵��� �� �ִ�.
		InterlockedDecrement(&_max_cnt);

		return true;
	}

	__forceinline long Size()
	{
		return _size;
	}

	long* SizePtr()
	{
		return &_size;
	}

private:

	Node* MaskNode(Node* mem_node)
	{
		uint64 cmp = _InterlockedIncrement(&_cmp_bit) << SHIFT_MASK47;
		return reinterpret_cast<Node*>(reinterpret_cast<size_t>(mem_node) | cmp);
	}

	Node* UnMaskNode(Node* mask_node)
	{
		return reinterpret_cast<Node*>(reinterpret_cast<size_t>(mask_node) & MEMORY_MASK);
	}

private:
	long _size;
	long _max_cnt;
	uint64 _cmp_bit;
	Node* _head;
	Node* _tail;
	uint64 _queue_descriptor;
	static LockFreeObjectPoolTLS<Node, true> node_pool;
};

template<typename T, int MAX_QUEUE_SIZE>
long LockFreeQueueStatic<T, MAX_QUEUE_SIZE>::g_queue_descriptor = 0;

template<typename T, int MAX_QUEUE_SIZE>
LockFreeObjectPoolTLS<typename LockFreeQueueStatic<T, MAX_QUEUE_SIZE>::Node, true> LockFreeQueueStatic<T, MAX_QUEUE_SIZE>::node_pool;

#pragma warning(default: 26495)
