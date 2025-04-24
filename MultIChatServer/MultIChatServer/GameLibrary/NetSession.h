#pragma once
#include "Values.h"
#include "LockFreeQueue.h"
#include "NetSerializeBuffer.h"
#include "NetRecvPacket.h"
#include "LockFreeQueueStatic.h"
#include "SocketUtils.h"
#include "LogUtils.h"
#include "NetSendPacket.h"
#include "SessionInstance.h"
#pragma warning (disable : 26495)
#pragma warning (disable : 4703)

class LogicInstance;

class NetSession
{
public:
	static const uint32 RELEASE_FLAG_OFF_MASK = ((uint32)1 << 31) - 1;
	static const uint32 RELEASE_FLAG_ON_MASK = ((uint32)1 << 31);

public:
	friend class NetService;

public:

	NetSession() : _send_flag(0), _io_cnt(0), _send_cnt(0), _disconnect_flag(1), _socket(INVALID_SOCKET), _recv_buf(nullptr) //< _time(0)
	{
		InterlockedOr(reinterpret_cast<long*>(&_io_cnt), static_cast<unsigned long>(RELEASE_FLAG_ON_MASK));
		
		//삭제시에는 항상 tick을 0으로 초기화한다.
		_time = 0;
		//Session OverlappedEvent 초기화
		_send_overlapped.instance = this;
		_send_overlapped.type = OverlappedEventType::SEND_OVERLAPPED;
		_recv_overlapped.instance = this;
		_post_overlapped.type = OverlappedEventType::SESSION_POST_OVERLAPPED;
		_post_overlapped.instance = this;

		_send_arr = new NetSerializeBuffer*[df_SEND_ARR_SIZE];
	}

	~NetSession()
	{
		if (_socket != INVALID_SOCKET)
		{
			closesocket(_socket);
		}

		delete[] _send_arr;

	}

	__forceinline void SessionNetPreInit(uint64 session_id)
	{
		_session_id = session_id;
		_send_flag = 0;
		_send_cnt = 0;
		_disconnect_flag = 0;
		_instance = nullptr;
		_logic_instance = nullptr;

		_recv_buf = ALLOC_NET_RECV_PACKET();
		_socket = SocketUtils::CreateSocket();
		memset(&_recv_overlapped, 0, sizeof(WSAOVERLAPPED));
		_recv_overlapped.type = OverlappedEventType::ACCEPT_OVERLAPPED;
	}

	__forceinline void SessionNetProcInit()
	{

		//TimeOut Thread 처리 -> session_id는 이미 사용중이므로 시간을 가져온 뒤 accept_flag 처리를 해준다
		_time = GetTickCount64();
		AtomicIncrement32(&_io_cnt);
		InterlockedAnd(reinterpret_cast<long*>(&_io_cnt), static_cast<unsigned long>(RELEASE_FLAG_OFF_MASK));
	}

	__forceinline void SessionNetTempInit()
	{
		_recv_overlapped.type = OverlappedEventType::MAXACCEPT_OVERLAPPED;
		_recv_buf = ALLOC_NET_RECV_PACKET();
		_socket = SocketUtils::CreateSocket();
	}

	__forceinline void SessionNetClear()
	{
		FREE_NET_RECV_PACKET(_recv_buf);

		//Debug Code
		_recv_buf = nullptr;

		for (int i = 0; i < _send_cnt; i++)
		{
			FREE_NET_SEND_PACKET(_send_arr[i]);
		}

		int size = _send_buf.Size();

		NetSerializeBuffer* buffer;

		for (int i = 0; i < size; i++)
		{
			_send_buf.Dequeue(buffer);
			FREE_NET_SEND_PACKET(buffer);
		}

		size = _msg_queue.Size();

		for (int i = 0; i < size; i++)
		{
			_msg_queue.Dequeue(buffer);
			FREE_NET_RECV_PACKET(buffer);
		}
		_time = 0;

		closesocket(_socket);
	}

	__forceinline void SessionNetTempClear()
	{
		closesocket(_socket);
		FREE_NET_RECV_PACKET(_recv_buf);
	}

	SOCKET GetSocket() { return _socket; }
	
	LockFreeQueueStatic<NetSerializeBuffer*>* GetMsgQueue() { return &_msg_queue; }
	void EnqueueMsg(NetSerializeBuffer* packet) { _msg_queue.Enqueue(packet); }

	void SetLogicInstance(LogicInstance* logic_instance) { InterlockedExchangePointer(reinterpret_cast<PVOID*>(&_logic_instance), reinterpret_cast<PVOID*>(logic_instance)); }
	LogicInstance* GetLogicInstance() { return reinterpret_cast<LogicInstance*>(InterlockedOr64(reinterpret_cast<LONG64*>(&_logic_instance), 0)); }
	
	void SetSessionInstance(SessionInstance* instance) {  _instance = instance; }
	SessionInstance* GetSessionInstance() { return _instance; }

	void AddCntInLib() { AtomicIncrement32(&_io_cnt); }
	int32* GetIoCntPtr() { return &_io_cnt; }

	uint64 GetSessionID() { return _session_id; }

	//삭제되었어도 문제가 없는 이유는 Send를 처리할 때 GetSession을 통해서 잡고 들어가기 때문에 문제가 되지 않는다.
	 bool SendPostFlag()
	{
		if ((_send_buf.Size() > 0) && (_send_flag == 0))
			return true;
		return false;
	}

private:
	ServiceType _type;
	SOCKET _socket;
	uint64 _session_id;
	char _send_flag;
	char _disconnect_flag;
	int32 _send_cnt;
	uint64 _time;
	LogicInstance* _logic_instance;
	SessionInstance* _instance;
	NetRecvPacket* _recv_buf;
	//정수값을 비교하는 것이 유리하다.
	OverlappedEvent _send_overlapped;
	OverlappedEvent _recv_overlapped;
	OverlappedEvent _post_overlapped;
	NetSerializeBuffer** _send_arr;
	LockFreeQueueStatic<NetSerializeBuffer*> _send_buf;
	LockFreeQueueStatic<NetSerializeBuffer*> _msg_queue;
	int32 _io_cnt;
};



#pragma warning (default : 26495)
#pragma warning (default : 4703)