#pragma once
#include "Lock.h"


class MonitorServer
{
public:

	MonitorServer() : _login_flag(false)
	{
		InitializeCriticalSection(&_monitor_cs);
	}

	~MonitorServer()
	{
		DeleteCriticalSection(&_monitor_cs);
	}

	bool CheckLogin(uint64 session_id)
	{
		CSLock lock_guard(&_monitor_cs);

		if (_login_flag == true)
			return false;

		_login_flag = true;
		_session_id = session_id;

		return true;
	}

	virtual bool UpdateData(uint8 data_type, int32 data_val, int32 time_stamp) abstract;

	__forceinline uint8 GetServerNo() { return _server_no; }
	__forceinline CRITICAL_SECTION* GetCS() { return &_monitor_cs; }
	__forceinline bool IsConnect() { return _login_flag; }
	__forceinline void SetLogin()
	{
		CSLock lock_guard(&_monitor_cs);
		_login_flag = true;
	}

	virtual bool Clear(uint64 session_id) abstract;

protected:
	uint8 _server_no;
	uint64 _session_id;
	CRITICAL_SECTION _monitor_cs;
	bool _login_flag;
};

