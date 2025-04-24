#pragma once
#include "MonitorServer.h"

class LoginMonitorServer : public MonitorServer
{
	friend class LanMonitor;

public:

	LoginMonitorServer()
	{
		_server_no = 1;
		ClearInfos();
	}

	__forceinline virtual bool UpdateData(uint8 data_type, int32 data_val, int32 time_stamp)
	{
		CSLock _lock_guard(&_monitor_cs);
		DataInfo* info;

		switch (data_type)
		{
		case dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN:
			info = &_login_infos.login_run;
			break;
		case dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU:
			info = &_login_infos.login_cpu;
			break;
		case dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM:
			info = &_login_infos.login_mem;
			break;
		case dfMONITOR_DATA_TYPE_LOGIN_SESSION:
			info = &_login_infos.login_session_num;
			break;
		case dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS:
			info = &_login_infos.login_auth_tps;
			break;
		case dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL:
			info = &_login_infos.login_packet_pool;
			break;
		default:
			return false;
		}

		info->total += data_val;
		info->cnt++;

		if (info->min_val > data_val)
			info->min_val = data_val;

		if (info->max_val < data_val)
			info->max_val = data_val;

		return true;
	}
	
	__forceinline bool CopyInfos(LoginServerInfo* login_infos)
	{
		CSLock _lock_guard(&_monitor_cs);
		if (_login_flag == false)
			return false;
		*login_infos = _login_infos;
		ClearInfos();
		return true;
	}

	__forceinline void ClearInfos()
	{
		_login_infos.login_auth_tps.Clear();
		_login_infos.login_cpu.Clear();
		_login_infos.login_mem.Clear();
		_login_infos.login_packet_pool.Clear();
		_login_infos.login_run.Clear();
		_login_infos.login_session_num.Clear();
	}

	virtual bool Clear(uint64 session_id)
	{
		CSLock _lock_guard(&_monitor_cs);
		if (_login_flag && (_session_id == session_id))
		{
			ClearInfos();
			_login_flag = false;
			_session_id = UINT16_MAX;
			return true;
		}

		return false;
	}
private:
	LoginServerInfo _login_infos;
};

