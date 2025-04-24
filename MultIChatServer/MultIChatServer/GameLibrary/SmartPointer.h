#pragma once
#include "NetSerializeBuffer.h"
#include "LanSerializeBuffer.h"


template <typename Type>
class SmartPointer
{
public:
	SmartPointer()
	{
	}

	SmartPointer(Type* ptr)
	{
		RefAdd(ptr);
	}

	SmartPointer(const SmartPointer& other)
	{
		RefAdd(other._ptr);
	}

	SmartPointer(SmartPointer&& other)
	{
		_ptr = other._ptr;
		other._ptr = nullptr;
	}


	~SmartPointer()
	{
		RefRemove();
	}

	SmartPointer& operator=(const SmartPointer<Type>& other)
	{
		if (this != &other)
		{
			RefRemove();
			RefAdd(other->_ptr);
		}
		return *this;
	}

	SmartPointer& operator=(SmartPointer<Type>&& other)
	{
		if (this != &other)
		{
			RefRemove();
			_ptr = other._ptr;
			other->_ptr = nullptr;
		}
		return *this;
	}

	Type& operator*()
	{
		return *_ptr;
	}

	Type* operator->()
	{
		return _ptr;
	}

	const Type& operator*() const
	{
		return *_ptr;
	}

	const Type* operator->() const
	{
		return _ptr;
	}

	bool IsEmpty()
	{
		return _ptr == nullptr;
	}

	Type* GetPtr()
	{
		return _ptr;
	}

private:

	void RefAdd(Type* ptr)
	{
		_ptr = ptr;
		if (_ptr != nullptr)
		{
			_ptr->AddCnt();
		}

	}

	void RefRemove()
	{
		if (_ptr != nullptr)
		{
			_ptr->RemoveCnt();
			_ptr = nullptr;
		}
	}

private:
	Type* _ptr = nullptr;
};

using NetSerializeBufferRef = SmartPointer<NetSerializeBuffer>;
using LanSerializeBufferRef = SmartPointer<LanSerializeBuffer>;
