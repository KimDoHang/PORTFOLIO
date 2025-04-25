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
            //반드시 저장해야 함 결과에 대한 체크가 필요하므로 이는 Atomic하게 처리가 된 이후이므로 언제든지 값이 변경될 여지가 존재한다.
            //즉 top 대신 new_node->next로 대체가 불가능하다.
            top = _top;
            new_node->next = top;

            //top의 경우 interlocked 후 변화를 비교해야하므로 저장이 필요하다.
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

            //굳이 top->next를 저장할 필요가 없는 것이 top이 동일하면 변경전이므로 top->next가 문제가 되지 않는다.
            if (_InterlockedCompareExchangePointer(reinterpret_cast<PVOID*>(&_top), top->next, mask_top) == mask_top)
            {
                break;
            }

        } while (true);

        //이동 생성자 존재하는 경우 이동 생성자로 처리
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
    //LockFreeStack의 경우 전체에서 하나이므로 각각의 cmp_bit, size에 대한 Interlocked 동기화 과정을 줄이기 위해서 이렇게 처리하였다.
    alignas(64) uint64 _cmp_bit;
    alignas(64) uint32 _size;
    alignas(64) LockFreeObjectPool<Node, true> _node_pool;

};

