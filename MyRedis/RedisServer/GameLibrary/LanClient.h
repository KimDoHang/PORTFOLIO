#pragma once
#include "NetAddress.h"
#include "SmartPointer.h"
#include "IocpCore.h"
#include "ThreadManager.h"

class LanClient
{

public:

#pragma warning(disable: 26495)

	LanClient() : _type(ServiceType::LanClientType)
	{
		_thread_manager = new ThreadManager;
	}

#pragma warning(default: 26495)

	~LanClient()
	{
		delete _thread_manager;
	}

	bool Init(const WCHAR* ip, SHORT port, int32 concurrent_thread_num, int32 max_thread_num, bool direct_io = true, bool nagle_on = false, int32 send_time = 0, int32 monitor_time = 0);
	void Start();
	void Stop();

public:
	void IOThreadLoop();
	void ConnectStart(bool direct_io, bool nagle_on);
public:
	virtual void OnConnect() abstract;
	virtual void OnRelease() abstract;
	virtual void OnRecvMsg(LanSerializeBuffer* packet) abstract;
	virtual void OnError(const int8 log_level, int32 err_code, const WCHAR* cause) abstract;
	virtual void OnExit() abstract;
	void PrintClientConsole();
public:
	bool SendPacket(LanSerializeBuffer* packet);
	bool SendPacket(LanSerializeBufferRef packet);
	void SendPush(LanSerializeBuffer* packet);
	bool Disconnect();
	bool SendDisconnect();
	void SendPostCall();
	void SendPostOnly();
public:

	SOCKADDR_IN* GetConnectServerAddr()
	{
		return &_connect_sever_addr;
	}

private:
	void CreateIOThread(int concurrent_thread_nums, int max_thread_nums);

private:
	void SendPost();
	void SendProc();
	void RecvPost();
	void RecvProc(DWORD transferred);
	void ReleasePost();
	void ReleaseProc();

	void DisconectInLib();
	bool GetSession();
public:
	ThreadManager* _thread_manager;
protected:
	ServiceType _type;
	IocpCore _iocp_core;
	SOCKADDR_IN _connect_sever_addr;
	OverlappedEvent _send_post_overlapped;
	int16 _send_time;
	LanClientSession _session;
	char _ip[20];
	uint16 _port;
	int32 _exit_io_thread_cnt;
protected:
	//data
	int32 _send_msg_tps;
	int32 _recv_msg_tps;
	int32 _send_msg_tick;
	int32 _recv_msg_tick;
	uint64 _send_msg_avg;
	uint64 _recv_msg_avg;
};

