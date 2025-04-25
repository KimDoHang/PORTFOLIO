#pragma once

//늘어나는 경우 어떻게 처리할 것인가?
class NetService;
class LanService;
class LanClient;
class NetClient;
enum MonitorInfoType : uint8
{
	NoneInfo = 0,
	AcceptInfoType = 1,
	IOInfoType = 2,
	UpdateInfoType = 3,
	TimeOutInfoType = 4,
	FrameInfoType = 5,
};


struct ThreadInfo
{
	ThreadInfo(HANDLE handle, DWORD thread_id, ServiceType service_type, MonitorInfoType thread_info_type) : _handle(handle), _thread_id(thread_id), _service_type(service_type), _thread_info_type(thread_info_type)
	{

	}

	HANDLE _handle;
	DWORD _thread_id;
	ServiceType _service_type;
	MonitorInfoType _thread_info_type;
};

class ThreadManager
{
public:

	ThreadManager();

	~ThreadManager();

	void Launch(unsigned int(*callback)(void* ptr), NetService* service, ServiceType service_type, MonitorInfoType info_type = MonitorInfoType::NoneInfo);
	void Launch(unsigned int(*callback)(void* ptr), LanService* service, ServiceType service_type, MonitorInfoType info_type = MonitorInfoType::NoneInfo);
	void Launch(unsigned int(*callback)(void* ptr), LanClient* service, ServiceType service_type, MonitorInfoType info_type = MonitorInfoType::NoneInfo);
	void Launch(unsigned int(*callback)(void* ptr), NetClient* service, ServiceType service_type, MonitorInfoType info_type = MonitorInfoType::NoneInfo);

	void Launch(unsigned int(*callback)(void* ptr), void* thread_instance, ServiceType service_type, MonitorInfoType info_type = MonitorInfoType::NoneInfo);

	void Join();
	void Start() { SetEvent(_start_event); }
	void Wait() { WaitForSingleObject(_start_event, INFINITE); }

	void SuspendThreadALL();
private:

	int32 _thread_idx;
	vector<ThreadInfo*> _threads;
	SRWLOCK _threadManager_srw;
	HANDLE _start_event;
};



