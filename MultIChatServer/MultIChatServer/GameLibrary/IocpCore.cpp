#include "pch.h"
#include "IocpCore.h"
#include "NetService.h"
#include "LanService.h"
#include "LanClient.h"
#include "NetClient.h"
#include "Lock.h"
#include "ThreadManager.h"

#pragma warning(disable : 6011)
#pragma warning(disable : 6387)

unsigned int IocpCore::NetIOThread(void* service_ptr)
{
	NetService* service = reinterpret_cast<NetService*>(service_ptr);
	service->IOThreadLoop();
	return 0;

}

unsigned int IocpCore::LanIOThread(void* service_ptr)
{
	LanService* service = reinterpret_cast<LanService*>(service_ptr);
	service->IOThreadLoop();
	return 0;

}

unsigned int IocpCore::LanClientIOThread(void* service_ptr)
{
	LanClient* service = reinterpret_cast<LanClient*>(service_ptr);
	service->IOThreadLoop();
	return 0;
}

unsigned int IocpCore::NetClientIOThread(void* service_ptr)
{
	NetClient* service = reinterpret_cast<NetClient*>(service_ptr);
	service->IOThreadLoop();
	return 0;
}

IocpCore::~IocpCore()
{
	CloseHandle(_iocp_handle);
}

void IocpCore::IOInit(LanService* service, int concurrent_thread_nums, int max_thread_nums)
{
	int32 iocp_handle_err = 0;

	_concurrent_thread_nums = concurrent_thread_nums;
	_thread_nums = max_thread_nums;

	_iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, concurrent_thread_nums);

	if (_iocp_handle == INVALID_HANDLE_VALUE)
	{
		service->OnError(dfLOG_LEVEL_SYSTEM, iocp_handle_err, L"IOCP HANDLE Create Fail [ErrCode:%d]");
		__debugbreak();
	}

}

void IocpCore::IOInit(NetService* service, int concurrent_thread_nums, int max_thread_nums)
{
	int32 iocp_handle_err = 0;

	_concurrent_thread_nums = concurrent_thread_nums;
	_thread_nums = max_thread_nums;

	_iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, concurrent_thread_nums);

	if (_iocp_handle == INVALID_HANDLE_VALUE)
	{
		service->OnError(dfLOG_LEVEL_SYSTEM, iocp_handle_err, L"IOCP HANDLE Create Fail [ErrCode:%d]");
		__debugbreak();
	}
}

void IocpCore::IOInit(LanClient* service, int concurrent_thread_nums, int max_thread_nums)
{
	int32 iocp_handle_err = 0;

	_concurrent_thread_nums = concurrent_thread_nums;
	_thread_nums = max_thread_nums;

	_iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, concurrent_thread_nums);

	if (_iocp_handle == INVALID_HANDLE_VALUE)
	{
		service->OnError(dfLOG_LEVEL_SYSTEM, iocp_handle_err, L"IOCP HANDLE Create Fail [ErrCode:%d]");
		__debugbreak();
	}


}

void IocpCore::IOInit(NetClient* service, int concurrent_thread_nums, int max_thread_nums)
{
	int32 iocp_handle_err = 0;

	_concurrent_thread_nums = concurrent_thread_nums;
	_thread_nums = max_thread_nums;

	_iocp_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, concurrent_thread_nums);

	if (_iocp_handle == INVALID_HANDLE_VALUE)
	{
		service->OnError(dfLOG_LEVEL_SYSTEM, iocp_handle_err, L"IOCP HANDLE Create Fail [ErrCode:%d]");
		__debugbreak();
	}

}





