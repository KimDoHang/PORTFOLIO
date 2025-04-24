#pragma once
#include "MonitorServer.h"

class HardwareMonitorServer : public MonitorServer
{
	friend class LanMonitor;

public:
	HardwareMonitorServer()
	{
		_server_no = 4;
		ClearInfos();
	}
	__forceinline virtual bool UpdateData(uint8 data_type, int32 data_val, int32 time_stamp)
	{
		CSLock _lock_guard(&_monitor_cs);
		DataInfo* info;

		switch (data_type)
		{
		case dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL:
			info = &_hardware_infos.hardware_cpu;
			break;
		case dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY:
			info = &_hardware_infos.hardware_mem;
			break;
		case dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV:
			info = &_hardware_infos.hardware_recv;
			break;
		case dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND:
			info = &_hardware_infos.hardware_send;
			break;
		case dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY:
			info = &_hardware_infos.hardware_available_mem;
			break;
		case dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RETRANSMISSION:
			info = &_hardware_infos.hardware_retransmission;
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

	__forceinline bool CopyInfos(HardwareInfo* hardware_info)
	{
		CSLock _lock_guard(&_monitor_cs);
		if (_login_flag == false)
			return false;
		*hardware_info = _hardware_infos;
		ClearInfos();
		return true;
	}

	__forceinline void ClearInfos()
	{
		_hardware_infos.hardware_available_mem.Clear();
		_hardware_infos.hardware_cpu.Clear();
		_hardware_infos.hardware_mem.Clear();
		_hardware_infos.hardware_recv.Clear();
		_hardware_infos.hardware_retransmission.Clear();
		_hardware_infos.hardware_send.Clear();
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
	HardwareInfo _hardware_infos;
};

