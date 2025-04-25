#pragma once
#include "ThreadManager.h"
#include "NetAddress.h"
#include "IocpCore.h"
#include "Values.h"
#include "Types.h"
#include "SmartPointer.h"

class NetClient
{
public:
#pragma warning(disable: 26495)

	NetClient() : _type(ServiceType::NetClientType)
	{
		_thread_manager = new ThreadManager;
	}
#pragma warning(default: 26495)

	~NetClient()
	{
		delete _thread_manager;
	}

	bool Init(const WCHAR* ip, SHORT port, int32 concurrent_thread_num, int32 max_thread_num, uint8 packet_code, uint8 packet_key, bool direct_io = true, bool nagle_on = false, int32 send_time = 0, int32 monitor_time = 0);
	void Start();
	void Stop();

public:
	void IOThreadLoop();
	void ConnectStart(bool direct_io, bool nagle_on);
public:
	virtual void OnConnect() abstract;
	virtual void OnRelease() abstract;
	virtual void OnRecvMsg(NetSerializeBuffer* packet) abstract;
	virtual void OnError(const int8 log_level, int32 err_code, const WCHAR* cause) abstract;
	virtual void OnExit() abstract;
	void PrintClientConsole();
public:
	bool SendPacket(NetSerializeBuffer* packet);
	bool SendPacket(NetSerializeBufferRef packet);
	void SendPush(NetSerializeBuffer* packet);
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

private:
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

protected:
	uint8 _packet_code;
	uint8 _packet_key;
protected:
	ServiceType _type;
	IocpCore _iocp_core;
	SOCKADDR_IN _connect_sever_addr;
	OverlappedEvent _send_post_overlapped;
	int16 _send_time;
	NetClientSession _session;
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

