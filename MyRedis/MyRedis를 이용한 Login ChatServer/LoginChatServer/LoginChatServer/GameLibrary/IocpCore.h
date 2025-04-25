#pragma once
#include "LanSession.h"
#include "NetSession.h"
#include "LanClientSession.h"
#include "NetClientSession.h"
#include "LogUtils.h"


class NetService;
class LanService;
class Lanclient;
struct ThreadInfo;

class IocpCore
{
	friend class NetService;
	friend class LanService;
	friend class LanClient;
	friend class NetClient;


public:

	//Thread static ÇÔ¼ö
	static unsigned int NetIOThread(void* service_ptr);
	static unsigned int LanIOThread(void* service_ptr);
	static unsigned int LanClientIOThread(void* service_ptr);
	static unsigned int NetClientIOThread(void* service_ptr);

#pragma warning (disable : 26495)
	IocpCore() : _iocp_handle(INVALID_HANDLE_VALUE) {}
#pragma warning (default : 26495)

	~IocpCore();

	__forceinline bool Register(LanClientSession* session)
	{
		return CreateIoCompletionPort(reinterpret_cast<HANDLE>(session->GetSocket()), _iocp_handle, 0, 0);
	}

	__forceinline bool Register(LanSession* session)
	{
		return CreateIoCompletionPort(reinterpret_cast<HANDLE>(session->GetSocket()), _iocp_handle, 0, 0);
	}

	__forceinline bool Register(NetSession* session)
	{
		return CreateIoCompletionPort(reinterpret_cast<HANDLE>(session->GetSocket()), _iocp_handle, 0, 0);
	}

	__forceinline bool Register(NetClientSession* session)
	{
		return CreateIoCompletionPort(reinterpret_cast<HANDLE>(session->GetSocket()), _iocp_handle, 0, 0);
	}

	__forceinline bool Register(SOCKET listen_socket)
	{
		return CreateIoCompletionPort(reinterpret_cast<HANDLE>(listen_socket), _iocp_handle, 0, 0);
	}

	void IOInit(NetService* service, int concurrent_thread_nums, int max_thread_nums);
	void IOInit(LanService* service, int concurrent_thread_nums, int max_thread_nums);
	void IOInit(LanClient* service, int concurrent_thread_nums, int max_thread_nums);
	void IOInit(NetClient* service, int concurrent_thread_nums, int max_thread_nums);

public:
	HANDLE _iocp_handle;
	int32 _concurrent_thread_nums;
	int32 _thread_nums;
};

