#include "pch.h"
#include "ThreadManager.h"
#include "Lock.h"

ThreadManager::ThreadManager() : _thread_idx(0)
{
	InitializeSRWLock(&_threadManager_srw);
	_start_event = CreateEvent(nullptr, true, false, nullptr);
}

ThreadManager::~ThreadManager()
{
	Join();
	{
		SrwLockExclusiveGuard lockguard(&_threadManager_srw);
		for (int i = 0; i < _threads.size(); i++)
		{
			CloseHandle(_threads[i]->_handle);
		}

		_threads.clear();
	}

	CloseHandle(_start_event);

}

//Monitor Class에 대한 동적 할당 및 해제 처리하기
void ThreadManager::Launch(unsigned int(*callback)(void* ptr), NetService* service, ServiceType service_type, MonitorInfoType info_type)
{
	{
		SrwLockExclusiveGuard lockguard(&_threadManager_srw);

		int32 thread_id = _thread_idx++;

		HANDLE thread = (HANDLE)_beginthreadex(nullptr, 0, callback, reinterpret_cast<void*>(service), 0, nullptr);

		ThreadInfo* thread_info = new ThreadInfo(thread, GetCurrentThreadId(), service_type, info_type);

		_threads.push_back(thread_info);
	}
}

void ThreadManager::Launch(unsigned int(*callback)(void* ptr), LanService* service, ServiceType service_type, MonitorInfoType info_type)
{
	{
		SrwLockExclusiveGuard lockguard(&_threadManager_srw);

		int32 thread_id = _thread_idx++;

		HANDLE thread = (HANDLE)_beginthreadex(nullptr, 0, callback, reinterpret_cast<void*>(service), 0, nullptr);

		ThreadInfo* thread_info = new ThreadInfo(thread, GetCurrentThreadId(), service_type, info_type);
		_threads.push_back(thread_info);
	}
}

void ThreadManager::Launch(unsigned int(*callback)(void* ptr), LanClient* service, ServiceType service_type, MonitorInfoType info_type)
{
	{
		SrwLockExclusiveGuard lockguard(&_threadManager_srw);

		int32 thread_id = _thread_idx++;

		HANDLE thread = (HANDLE)_beginthreadex(nullptr, 0, callback, reinterpret_cast<void*>(service), 0, nullptr);

		ThreadInfo* thread_info = new ThreadInfo(thread, GetCurrentThreadId(), service_type, info_type);
		_threads.push_back(thread_info);
	}

}

void ThreadManager::Launch(unsigned int(*callback)(void* ptr), NetClient* service, ServiceType service_type, MonitorInfoType info_type)
{
	{
		SrwLockExclusiveGuard lockguard(&_threadManager_srw);

		int32 thread_id = _thread_idx++;

		HANDLE thread = (HANDLE)_beginthreadex(nullptr, 0, callback, reinterpret_cast<void*>(service), 0, nullptr);

		ThreadInfo* thread_info = new ThreadInfo(thread, GetCurrentThreadId(), service_type, info_type);
		_threads.push_back(thread_info);
	}

}

void ThreadManager::Launch(unsigned int(*callback)(void* ptr), void* thread_instance, ServiceType service_type, MonitorInfoType info_type)
{
	{
		SrwLockExclusiveGuard lockguard(&_threadManager_srw);

		int32 thread_id = _thread_idx++;

		HANDLE thread = (HANDLE)_beginthreadex(nullptr, 0, callback, reinterpret_cast<void*>(thread_instance), 0, nullptr);

		ThreadInfo* thread_info = new ThreadInfo(thread, GetCurrentThreadId(), service_type, info_type);
		_threads.push_back(thread_info);
	}
}

void ThreadManager::Join()
{
	vector<HANDLE> handles;
	int size;
	{
		SrwLockExclusiveGuard lockguard(&_threadManager_srw);
		size = static_cast<int>(_threads.size());

		for (int i = 0; i < size; i++)
		{
			handles.push_back(_threads[i]->_handle);
		}
	}

	WaitForMultipleObjects(size, handles.data(), true, INFINITE);
}

void ThreadManager::SuspendThreadALL()
{
	HANDLE h_thread = GetCurrentThread();

	SrwLockExclusiveGuard lockguard(&_threadManager_srw);
	for (int i = 0; i < _threads.size(); i++)
	{
		if (_threads[i] == h_thread)
			continue;

		SuspendThread(_threads[i]->_handle);
	}

}

