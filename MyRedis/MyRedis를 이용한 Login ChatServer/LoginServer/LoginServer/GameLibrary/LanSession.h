#pragma once
#include "Values.h"
#include "LockFreeQueue.h"
#include "LanSerializeBuffer.h"
#include "LanRecvPacket.h"
#include "LockFreeQueueStatic.h"
#include "SocketUtils.h"
#include "LogUtils.h"
#include "LanSendPacket.h"

#pragma warning (disable : 26495)
#pragma warning (disable : 4703)


class LanSession
{
public:
	static const uint32 RELEASE_FLAG_OFF_MASK = ((uint32)1 << 31) - 1;
	static const uint32 RELEASE_FLAG_ON_MASK = ((uint32)1 << 31);

public:
	friend class LanService;

public:

	LanSession() : _send_flag(0), _io_cnt(0), _send_cnt(0), _disconnect_flag(1), _time(0), _socket(INVALID_SOCKET), _recv_buf(nullptr)
	{
		InterlockedOr(reinterpret_cast<long*>(&_io_cnt), static_cast<unsigned long>(RELEASE_FLAG_ON_MASK));
		_send_overlapped.instance = this;
		_send_overlapped.type = OverlappedEventType::SEND_OVERLAPPED;
		_recv_overlapped.instance = this;
		_session_id = UINT16_MAX;
		_send_arr = new LanSerializeBuffer * [df_SEND_ARR_SIZE];
	}

	~LanSession()
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

		//accept_flag를 확인 후에 시간을 비교하므로 그 전에 값을 가져와야만 한다.
		_time = GetTickCount64();
		AtomicIncrement32(&_io_cnt);
		InterlockedAnd(reinterpret_cast<long*>(&_io_cnt), static_cast<unsigned long>(RELEASE_FLAG_OFF_MASK));
	}

	__forceinline void SessionLanClear()
	{
		FREE_LAN_RECV_PACKET(_recv_buf);

		//Debug Code
		_recv_buf = nullptr;

		for (int i = 0; i < _send_cnt; i++)
		{
			FREE_LAN_SEND_PACKET(_send_arr[i]);
		}

		int size = _send_buf.Size();

		LanSerializeBuffer* buffer;

		for (int i = 0; i < size; i++)
		{
			_send_buf.Dequeue(buffer);
			FREE_LAN_SEND_PACKET(buffer);
		}
	}


	__forceinline void SessionNetTempInit()
	{
		_recv_overlapped.type = OverlappedEventType::MAXACCEPT_OVERLAPPED;
		_recv_buf = ALLOC_LAN_RECV_PACKET();
		_socket = SocketUtils::CreateSocket();
	}

	__forceinline void SessionNetTempClear()
	{
		closesocket(_socket);
		FREE_LAN_RECV_PACKET(_recv_buf);
	}

	__forceinline SOCKET GetSocket() { return _socket; }

private:
	ServiceType _type;
	SOCKET _socket;
	uint64 _session_id;
	char _send_flag;
	char _disconnect_flag;
	int32 _send_cnt;
	uint64 _time;
	LanRecvPacket* _recv_buf;
	//정수값을 비교하는 것이 유리하다.
	OverlappedEvent _send_overlapped;
	OverlappedEvent _recv_overlapped;
	int32 _io_cnt;
	LanSerializeBuffer** _send_arr;
	LockFreeQueueStatic<LanSerializeBuffer*> _send_buf;
};

#pragma warning (default : 26495)
#pragma warning (default : 4703)


