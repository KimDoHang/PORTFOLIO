#pragma once
#include "Values.h"
#include "LockFreeQueue.h"
#include "NetSerializeBuffer.h"
#include "NetRecvPacket.h"
#include "LockFreeQueueStatic.h"
#include "SocketUtils.h"
#include "LogUtils.h"
#include "NetSendPacket.h"

class NetClientSession
{
public:
	friend class NetClient;

	static const uint32 RELEASE_FLAG_OFF_MASK = ((uint32)1 << 31) - 1;
	static const uint32 RELEASE_FLAG_ON_MASK = ((uint32)1 << 31);

public:

	NetClientSession() : _type(ServiceType::NetClientType), _send_flag(0), _io_cnt(0), _send_cnt(0), _disconnect_flag(1), _time(0), _socket(INVALID_SOCKET), _recv_buf(nullptr)
	{
		InterlockedOr(reinterpret_cast<long*>(&_io_cnt), static_cast<unsigned long>(RELEASE_FLAG_ON_MASK));
		_send_overlapped.instance = this;
		_send_overlapped.type = OverlappedEventType::SEND_OVERLAPPED;
		_recv_overlapped.instance = this;
		_send_arr = new NetSerializeBuffer * [df_SEND_ARR_SIZE];

	}

	~NetClientSession()
	{
		if (_socket != INVALID_SOCKET)
		{
			closesocket(_socket);
		}

		delete[] _send_arr;
	}

	__forceinline void SessionProcInit()
	{
		_send_flag = 0;
		_send_cnt = 0;
		_disconnect_flag = 0;
		_recv_buf = ALLOC_NET_RECV_PACKET();
		//accept_flag를 확인 후에 시간을 비교하므로 그 전에 값을 가져와야만 한다.
		_time = GetTickCount64();
		AtomicIncrement32(&_io_cnt);
		InterlockedAnd(reinterpret_cast<long*>(&_io_cnt), static_cast<unsigned long>(RELEASE_FLAG_OFF_MASK));
	}

	__forceinline void SessionLanClear()
	{

		//Debug Code
		if (_recv_buf == nullptr)
		{
			__debugbreak();
		}

		FREE_NET_RECV_PACKET(_recv_buf);

		_recv_buf = nullptr;


		for (int i = 0; i < _send_cnt; i++)
		{
			_send_arr[i]->RemoveCnt();
		}

		int size = _send_buf.Size();

		NetSerializeBuffer* buffer;

		for (int i = 0; i < size; i++)
		{
			_send_buf.Dequeue(buffer);
			buffer->RemoveCnt();
		}

		if (_socket != INVALID_SOCKET)
			closesocket(_socket);
	}

	__forceinline SOCKET GetSocket() { return _socket; }

private:
	ServiceType _type;
	SOCKET _socket;
	char _send_flag;
	char _disconnect_flag;

	int32 _send_cnt;
	uint64 _time;
	NetRecvPacket* _recv_buf;
	//정수값을 비교하는 것이 유리하다.
	OverlappedEvent _send_overlapped;
	OverlappedEvent _recv_overlapped;
	int32 _io_cnt;
	NetSerializeBuffer** _send_arr;
	LockFreeQueueStatic<NetSerializeBuffer*> _send_buf;
};


