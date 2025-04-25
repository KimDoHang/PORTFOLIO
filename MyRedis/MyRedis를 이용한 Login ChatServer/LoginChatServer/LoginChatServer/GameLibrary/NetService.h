#pragma once

#include "NetAddress.h"
#include "IocpCore.h"
#include "Values.h"
#include "Types.h"
#include "SmartPointer.h"
#include "LockFreeStack.h"
#include "Kicker.h"
#include "FrameTimer.h"

#pragma warning (disable : 26495)

class LogicInstance;
class GroupInstance;
class LoopInstance;
class NetObserver;

class NetService
{

public:
	NetService();
	~NetService();

public:
	//�ܺο��� ����ϴ� NetService �Լ�
	bool Init(const WCHAR* ip, SHORT port, int32 concurrent_thread_num, int32 max_thread_num, uint8 packet_code, uint8 packet_key, bool direct_io = true, bool nagle_on = false, int32 max_session_num = INT32_MAX, int32 accept_job_cnt = 5, int32 monitor_time = 0, int32 timeout_time = 0, int32 backlog_size = 255);
	void Start();
	void Stop();

public:
	//content ��� - Callback
	virtual bool OnAccept(uint64 session_id) abstract;
	virtual void OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause) abstract;
	virtual void OnError(const int8 log_level, int32 err_code, const WCHAR* cause) abstract;
	virtual bool OnConnectionRequest(WCHAR* IP, SHORT port) abstract;
	virtual bool OnTimeOut(uint64 thread_id) abstract;
	virtual void OnMonitor() abstract;
	virtual void OnExit() abstract;

public:
	void PrintLibraryConsole();
public:
	
	//Content Logic Arr ���� �Լ���
	void CreateLogicInstanceArr(uint16 max_content_size);
	void AttachLogicInstance(uint16 logic_id, LogicInstance* logic_instance);
	bool DetachGroupInstance(uint16 logic_id);

	//���� �Լ�
	void StartLoopInstance(LoopInstance* loop_instance);
	void ExitLoopInstance();
	LogicInstance* GetLogicInstance(uint16 logic_id);

	void RegisterLoop(LoopInstance* loop_instance);
	//Content ��ü�� Session�� ����ϴ� �Լ�
	bool RegisterSessionInstance(uint64 session_id, SessionInstance* instance);
public:
	//�ܺ� I/O ó�� �Լ�
	bool SendPacket(uint64 session_id, NetSerializeBufferRef packet);
	bool SendPacket(uint64 session_id, NetSerializeBuffer* packet);
	bool SendPacketGroup(uint64 session_id, NetSerializeBuffer* packet);
	bool SendDisconnect(uint64 session_id, NetSerializeBuffer* packet);
	bool SendDisconnectGroup(uint64 session_id, NetSerializeBuffer* packet);

	__forceinline void RegisterSessionSendPost(uint64 session_id)
	{
		NetSession* session = GetSession(session_id);

		if (session == nullptr)
		{
			return;
		}

		PostQueuedCompletionStatus(_iocp_core._iocp_handle, 0, 0, reinterpret_cast<OVERLAPPED*>(&session->_post_overlapped));
	}

	bool Disconnect(uint64 session_id);
	void ReturnSession(NetSession* session);
	void DisconectInLib(NetSession* session);

	void ReleasePost(NetSession* session);
	void ReleaseProc(NetSession* session);

	//Session ó�� �Լ�
	NetSession* GetSession(uint64 session_id);

	__forceinline NetSession* GetSessionInLib(uint64 session_id)
	{
		return &_sessions[static_cast<SHORT>(session_id)];
	}

public:
	//Utils �Լ�
	IocpCore* GetIocpCore()
	{
		return &_iocp_core;
	}

	SOCKADDR_IN* GetServerAddr()
	{
		return &_server_addr;
	}

	void ConsoleSize(int32 row, int32 col)
	{
		char console[100];
		sprintf_s(console, 100, " mode  con lines=%d   cols=%d ", row, col);
		system(console);
	}

	//������ IP, PORT�� �ѹ� �˾Ƴ����Ƿ� ���� ���Ŀ��� �� �ʿ䰡 ����.
	//Content�ʿ��� �ʿ��ϴٸ� �̸� �����ϰ� �α׸� ���� ���� OnError�� Session_id�� ���� player�� ã�� ���� �������� �����ϴ� ����
	//ȿ�����̶�� �����Ͽ���.
	__forceinline bool GetSessionAddr(uint64 session_id, SOCKADDR_IN* socket_addr)
	{
		int32 err_code = 0;
		NetSession* session = GetSession(session_id);

		if (session == nullptr)
			return false;

		int32 sockaddr_size = sizeof(sockaddr_in);
		if (::getpeername(session->_socket, reinterpret_cast<SOCKADDR*>(socket_addr), &sockaddr_size) == SOCKET_ERROR)
		{

			OnError(dfLOG_LEVEL_SYSTEM, session->_session_id, err_code, L"ClientSocket Peer Name Fail");

			ReturnSession(session);
			return false;
		}

		ReturnSession(session);
		return true;
	}
public:
	//Utils CreateThread
	void CreateIOThread(int concurrent_thread_num, int max_thread_num);
	void CreateTimeOutThread(int32 timeout_time);
	void CreateObserverThread(int32 observer_time);
	void CreateFrameTimerThread();

	//Thread�� ���� ȣ���� ���� ���� ������ �Լ�
	void IOThreadLoop();
	bool TimeOutThreadLoop();
	bool ObserverThreadLoopIOCP();

	//Frame Timer Thread ȣ�� �Լ�
	void FrameTimerThreadLoop();
	uint32 PostFrame();

private:
	//�⺻ I/O ó�� �Լ�
	void AcceptStart(bool direct_io, bool nagle_on, int32 backlog_size, int32 accept_job_cnt);
	void SendPost(NetSession* session);
	void SendProc(NetSession* session);
	void RecvPost(NetSession* session);
	void RecvProc(NetSession* session, DWORD transferred);
	void AcceptPost();
	bool AcceptProc(NetSession* session);
	void MaxAcceptProc(NetSession* session);
	void ResetProc(NetSession* session);
private:
	__forceinline bool SetDisconnectPacket(NetSerializeBuffer* buffer)
	{
		if (SetPacket(buffer) == false)
			return false;

		AtomicExchange8(&buffer->_send_disconnect_flag, 1);
		return true;
	}

	__forceinline bool SetPacket(NetSerializeBuffer* buffer)
	{
		if (InterlockedExchange8(reinterpret_cast<char*>(&buffer->_encryption), 1) == 1)
			return false;

		NetHeader* header = reinterpret_cast<NetHeader*>(buffer->_chpBuffer);
		header->_code = _packet_code;
		int32 data_len = header->_len = buffer->GetDataSize();
		uint16 rand_seed = header->_rand_seed = rand() % 256;

		header->_check_sum = 0;

		char* data = buffer->_chpBuffer + offsetof(NetHeader, _check_sum);

		for (int i = 1; i < data_len + 1; i++)
		{
			data[0] += data[i];
		}

		uint8 p, e;

		p = data[0] ^ (rand_seed + 1);
		e = p ^ (_packet_key + 1);
		data[0] = e;

		for (int i = 1; i < data_len + 1; i++)
		{
			p = data[i] ^ (rand_seed + i + 1 + p);
			e = p ^ (_packet_key + i + 1 + data[i - 1]);
			data[i] = e;
		}

		return true;
	}

	__forceinline bool GetPacket(NetSerializeBuffer* buffer)
	{
		char* data = buffer->_chpBuffer + offsetof(NetHeader, _check_sum);

		uint8 p = 0;
		uint8 prev_p = 0;
		uint8 e = 0;
		uint8 check_sum = 0;
		uint8 rand_seed = reinterpret_cast<NetHeader*>(buffer->_chpBuffer)->_rand_seed;
		int32 data_len = buffer->GetDataSize();

		for (int i = 0; i < data_len + 1; i++)
		{
			prev_p = p;
			p = data[i] ^ (e + _packet_key + i + 1);
			e = data[i];
			data[i] = p ^ (prev_p + rand_seed + i + 1);
			check_sum += data[i];
		}

		check_sum -= data[0];

		if (check_sum != reinterpret_cast<NetHeader*>(buffer->_chpBuffer)->_check_sum)
			return false;

		return true;
	}
public:
	ThreadManager* _thread_manager;
	NetObserver* _observer;
protected:
	uint8 _packet_code;
	uint8 _packet_key;
protected:
	SOCKET _listen_socket;
	IocpCore _iocp_core;
protected:
	char _exit_flag;
protected:
	//Session �迭
	NetSession* _sessions;
	//MAX Session Num
	int32 _max_session_num;
	//session_idx�� ���� cnt
	uint64 _session_cnt;
	//Stop ó���� ���� ��ü Accept ó�� ��, �Ϸ� session num
	int64 _alloc_session_num;
	//���� session_num ���� -> Accept �񵿱�� ���� ���� ���� �ʿ�
	int32 _cur_session_num;
	LockFreeObjectPool<NetSession, false>* _session_pool;
	LockFreeStack<uint16> _session_idx_stack;
protected:
	Kicker* _kicker;
	FrameTimer _frame_timer;
	LogicInstance** _logic_content_arr;
	uint16 _logic_content_arr_size;
	int16 _logic_instance_num;
protected:
	ServiceType _type;
	SOCKADDR_IN _server_addr;
	char _ip[20];
	uint16 _port;
};

#pragma warning (default : 26495)
