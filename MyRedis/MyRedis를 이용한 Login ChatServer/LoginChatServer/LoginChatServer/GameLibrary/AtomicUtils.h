#pragma once

__forceinline static int16 AtomicIncrement16(int16* num)
{
	return _InterlockedIncrement16(reinterpret_cast<short*>(num));
}

__forceinline static uint16 AtomicIncrement16(uint16* num)
{
	return static_cast<uint16>(_InterlockedIncrement16(reinterpret_cast<short*>(num)));
}

__forceinline static int32 AtomicIncrement32(int32* num)
{
	return _InterlockedIncrement(reinterpret_cast<long*>(num));
}

__forceinline static uint32 AtomicIncrement32(uint32* num)
{
	return _InterlockedIncrement(reinterpret_cast<unsigned long*>(num));
}

__forceinline static int64 AtomicIncrement64(int64* num)
{
	return _InterlockedIncrement64(reinterpret_cast<long long*>(num));
}

__forceinline static uint64 AtomicIncrement64(uint64* num)
{
	return static_cast<uint64>(_InterlockedIncrement64(reinterpret_cast<long long*>(num)));
}

__forceinline static int16 AtomicDecrement16(int16* num)
{
	return _InterlockedDecrement16(reinterpret_cast<short*>(num));
}

__forceinline static uint16 AtomicDecrement16(uint16* num)
{
	return static_cast<uint16>(_InterlockedDecrement16(reinterpret_cast<short*>(num)));
}

__forceinline static int32 AtomicDecrement32(int32* num)
{
	return _InterlockedDecrement(reinterpret_cast<long*>(num));
}

__forceinline static uint32 AtomicDecrement32(uint32* num)
{
	return _InterlockedDecrement(reinterpret_cast<unsigned long*>(num));
}

__forceinline static int64 AtomicDecrement64(int64* num)
{
	return _InterlockedDecrement64(reinterpret_cast<long long*>(num));
}

__forceinline static uint64 AtomicDecrement64(uint64* num)
{
	return static_cast<uint64>(_InterlockedIncrement64(reinterpret_cast<long long*>(num)));
}

__forceinline static int8 AtomicExchange8(int8* num, int8 val)
{
	return _InterlockedExchange8(reinterpret_cast<CHAR*>(num), val);
}

__forceinline static int32 AtomicExchange32ToZero(int32* num)
{
	return _InterlockedExchange(reinterpret_cast<long*>(num), 0);
}

__forceinline static uint32 AtomicExchange32ToZero(uint32* num)
{
	return _InterlockedExchange(reinterpret_cast<unsigned long*>(num), 0);
}

__forceinline static int64 AtomicExchange64ToZero(int64* num)
{
	return _InterlockedExchange64(reinterpret_cast<long long*>(num), 0);
}

__forceinline static uint64 AtomicExchange64ToZero(uint64* num)
{
	return _InterlockedExchange64(reinterpret_cast<long long*>(num), 0);
}
