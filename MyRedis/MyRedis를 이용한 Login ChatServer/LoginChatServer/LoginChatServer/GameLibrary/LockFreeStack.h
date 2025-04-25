#pragma once
#include "LockFreeObjectPool.h"

template<typename T>
class LockFreeStack
{

    const uint64 SHIFT_MASK47 = 47;
    const uint64 MEMORY_MASK = ((uint64)1 << 47) - 1;

    struct Node
    {
        Node(T&& data) : data(std::forward<T>(data)) {}
        T data;
        Node* next = nullptr;
    };

public:

    LockFreeStack() : _top(nullptr), _cmp_bit(0), _size(0)
    {

    }

    ~LockFreeStack()
    {
        Node* tempNode;

        while (_top != nullptr)
        {
            tempNode = UnMaskNode(_top);
            _top = tempNode->next;
            free(tempNode);
        }
    }

    void push(T data)
    {
        Node* new_node = _node_pool.Alloc(std::move(data));

        Node* mask_new_node = MaskNode(new_node);
        Node* top;

        do
        {
            //�ݵ�� �����ؾ� �� ����� ���� üũ�� �ʿ��ϹǷ� �̴� Atomic�ϰ� ó���� �� �����̹Ƿ� �������� ���� ����� ������ �����Ѵ�.
            //�� top ��� new_node->next�� ��ü�� �Ұ����ϴ�.
            top = _top;
            new_node->next = top;

            //top�� ��� interlocked �� ��ȭ�� ���ؾ��ϹǷ� ������ �ʿ��ϴ�.
            if (_InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_top), mask_new_node, top) == top)
            {
                break;
            }

        } while (true);

        AtomicIncrement32(&_size);
    }

    bool pop(T& data)
    {
        Node* mask_top;
        Node* top;

        long size = AtomicDecrement32(&_size);

        if (size < 0)
        {
            size = AtomicIncrement32(&_size);
            return false;
        }

        do
        {
            mask_top = _top;
            top = UnMaskNode(mask_top);

            //���� top->next�� ������ �ʿ䰡 ���� ���� top�� �����ϸ� �������̹Ƿ� top->next�� ������ ���� �ʴ´�.
            if (_InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_top), top->next, mask_top) == mask_top)
            {
                break;
            }

        } while (true);

        //�̵� ������ �����ϴ� ��� �̵� �����ڷ� ó��
        data = std::move(top->data);
        _node_pool.Free(top);

        return true;
    }

    long Size()
    {
        return _size;
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


public:
    alignas(64) Node* _top = nullptr;
    //LockFreeStack�� ��� ��ü���� �ϳ��̹Ƿ� ������ cmp_bit, size�� ���� Interlocked ����ȭ ������ ���̱� ���ؼ� �̷��� ó���Ͽ���.
    alignas(64) uint64 _cmp_bit;
    alignas(64) uint32 _size;
    alignas(64) LockFreeObjectPool<Node, true> _node_pool;

};

