#pragma once
#include "NetAddress.h"
#include "IocpCore.h"
#include "Values.h"
#include "Types.h"
#include "SmartPointer.h"
#include "LockFreeStack.h"
#include "Kicker.h"
#include "LanObserver.h"

class LanService
{

public:
	LanService();
	~LanService();

public:
	//�ܺο��� ����ϴ� NetService �Լ�
	bool Init(const WCHAR* ip, SHORT port, int32 concurrent_thread_num, int32 max_thread_num, bool direct_io = true, bool nagle_onoff = false, int32 max_session_num = INT32_MAX, int32 accept_job_cnt = 2, int32 send_time = 0, int32 monitor_time = 0, int32 backlog_size = 255);
	void Start();
	void Stop();

public:
	//content ��� - Callback
	virtual bool OnAccept(uint64 session_id) abstract;
	virtual void OnRelease(uint64 session_id) abstract;
	virtual void OnRecvMsg(uint64 session_id, LanSerializeBuffer* packet) abstract;
	virtual void OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause) abstract;
	virtual void OnError(const int8 log_level, int32 err_code, const WCHAR* cause) abstract;
	virtual bool OnConnectionRequest(WCHAR* IP, SHORT port) abstract;
	virtual bool OnTimeOut(uint64 thread_id) abstract;
	virtual void OnMonitor() abstract;
	virtual void OnExit() abstract;

public:
	static unsigned int TimeOutThread(void* service_ptr);
public:
	//Utils CreateThread
	void CreateIOThread(int concurrent_thread_num, int max_thread_num);
	void CreateObserverThread(int32 monitor_time);
	void CreateTimeOutThread(int32 timeout_time);
public:
	//Thread�� ���� ȣ���� ���� ���� ������ �Լ�
	void IOThreadLoop();
	void ObserverThreadLoop();
	void TimeOutThreadLoop();
	void PrintLibraryConsole();
	char GetExitFlag()
	{
		return _exit_flag;
	}
public:
	//�ܺ� I/O ó�� �Լ�
	bool SendPacket(uint64 session_id, LanSerializeBuffer* packet);
	bool SendPacket(uint64 session_id, LanSerializeBufferRef packet);
	void SendPostCall();
	bool Disconnect(uint64 session_id);
	bool SendDisconnect(uint64 session_id);

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

	__forceinline bool GetSessionAddr(uint64 session_id, SOCKADDR_IN* socket_addr)
	{
		int32 err_code = 0;
		LanSession* session = GetSession(session_id);

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

	int32 GetSessionArrNum() { return _max_session_num; }


private:
	//�⺻ I/O ó�� �Լ�
	void AcceptStart(bool direct_io, bool nagle_onoff, int32 backlog_size, int32 accept_job_cnt);
	void SendPost(LanSession* session);
	void SendProc(LanSession* session);
	void RecvPost(LanSession* session);
	void RecvProc(LanSession* session, DWORD transferred);
	void AcceptPost();
	void MaxAcceptProc(LanSession* session);
	bool AcceptProc(LanSession* session);
	void ReleasePost(LanSession* session);
	void ResetProc(LanSession* session);
	void ReleaseProc(LanSession* session);
	void SendPostOnly();
private:
	//Session ó�� �Լ�
	LanSession* GetSession(uint64 session_id);
	void ReturnSession(LanSession* session);
	void DisconectInLib(LanSession* session);
public:
	ThreadManager* _thread_manager;
	LanObserver _observer;
private:
	ServiceType _type;
private:
	char _exit_flag;
	SOCKET _listen_socket;
	IocpCore _iocp_core;
	int32 _send_time;
	int32 _timeout_time;
private:
	LanSession* _sessions;
	int32 _max_session_num;
	//job cnt�� ���� �迭 num
	OverlappedEvent _send_post_overlapped;
protected:
	//Acceptor ������ ��� ����
	//session_idx�� ���� cnt
	uint64 _session_cnt;
	//Stop ó���� ���� ��ü Accept ó�� ��, �Ϸ� session num
	int64 _alloc_session_num;
	//���� session_num ���� -> Accept �񵿱�� ���� ���� ���� �ʿ�
	int32 _cur_session_num;
private:
	LockFreeObjectPool<LanSession, false>* _session_pool;
	LockFreeStack<uint16> _session_idx_stack;
private:
	SOCKADDR_IN _server_addr;
	char _ip[20];
	uint16 _port;
};

