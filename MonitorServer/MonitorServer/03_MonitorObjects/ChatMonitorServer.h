#pragma once
#include "MonitorServer.h"
#include "Values.h"
class ChatMonitorServer : public MonitorServer
{
	friend class LanMonitor;
public:

	ChatMonitorServer()
	{
		_server_no = 2;
		ClearInfos();
	}

	__forceinline virtual bool UpdateData(uint8 data_type, int32 data_val, int32 time_stamp)
	{
		CSLock _lock_guard(&_monitor_cs);
		DataInfo* info;

		switch (data_type)
		{
		case dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN:
			info = &_chat_infos.chat_run;
			break;
		case dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU:
			info = &_chat_infos.chat_cpu;
			break;
		case dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM:
			info = &_chat_infos.chat_mem;
			break;
		case dfMONITOR_DATA_TYPE_CHAT_SESSION:
			info = &_chat_infos.chat_session_num;
			break;
		case dfMONITOR_DATA_TYPE_CHAT_PLAYER:
			info = &_chat_infos.chat_player_num;
			break;
		case dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS:
			info = &_chat_infos.chat_update_tps;
			break;
		case dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL:
			info = &_chat_infos.chat_packet_pool;
			break;
		case dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL:
			info = &_chat_infos.chat_msg_pool;
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


	__forceinline bool CopyInfos(ChatServerInfo* chat_infos)
	{
		CSLock _lock_guard(&_monitor_cs);
		if (_login_flag == false)
			return false;

		*chat_infos = _chat_infos;
		ClearInfos();
		return true;
	}

	bool UpdateSector(uint64 session_id, uint8 server_no, uint16 sector_infos[][MAX_SECTOR_SIZE_X])
	{
		CSLock _lock_guard(&_monitor_cs);

		if (_login_flag == false || (_session_id != session_id))
		{
			return false;
		}

		for (int y = 0; y < MAX_SECTOR_SIZE_Y; y++)
		{
			for (int x = 0; x < MAX_SECTOR_SIZE_X; x++)
			{
				_sector_infos[y][x] = sector_infos[y][x];
			}
		}

		return true;
	}


	__forceinline void ClearInfos()
	{
		_chat_infos.chat_run.Clear();
		_chat_infos.chat_cpu.Clear();
		_chat_infos.chat_mem.Clear();
		_chat_infos.chat_session_num.Clear();
		_chat_infos.chat_player_num.Clear();
		_chat_infos.chat_update_tps.Clear();
		_chat_infos.chat_packet_pool.Clear();
		_chat_infos.chat_msg_pool.Clear();
		ClearSector();
	}

	void ClearSector()
	{
		{
			for (int y = 0; y < MAX_SECTOR_SIZE_Y; y++)
			{
				for (int x = 0; x < MAX_SECTOR_SIZE_X; x++)
				{
					_sector_infos[y][x] = 0;
				}
			}
		}
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
	ChatServerInfo _chat_infos;
	uint16 _sector_infos[MAX_SECTOR_SIZE_Y][MAX_SECTOR_SIZE_X];
};

