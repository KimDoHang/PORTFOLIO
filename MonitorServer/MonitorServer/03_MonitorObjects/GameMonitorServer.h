#pragma once
#include "MonitorServer.h"

class GameMonitorServer : public MonitorServer
{
	friend class LanMonitor;

public:

	GameMonitorServer()
	{
		_server_no = 3;
		ClearInfos();
	}

	__forceinline virtual bool UpdateData(uint8 data_type, int32 data_val, int32 time_stamp)
	{
		CSLock _lock_guard(&_monitor_cs);
		DataInfo* info;

		switch (data_type)
		{
		case dfMONITOR_DATA_TYPE_GAME_SERVER_RUN:
			info = &_game_infos.game_run;
			break;
		case dfMONITOR_DATA_TYPE_GAME_SERVER_CPU:
			info = &_game_infos.game_cpu;
			break;
		case dfMONITOR_DATA_TYPE_GAME_SERVER_MEM:
			info = &_game_infos.game_mem;
			break;
		case dfMONITOR_DATA_TYPE_GAME_SESSION:
			info = &_game_infos.game_session_num;
			break;
		case dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER:
			info = &_game_infos.game_auth_num;
			break;
		case dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER:
			info = &_game_infos.game_player_num;
			break;
		case dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS:
			info = &_game_infos.game_accept_tps;
			break;
		case dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS:
			info = &_game_infos.game_recv_tps;
			break;
		case dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS:
			info = &_game_infos.game_send_tps;
			break;
		case dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS:
			info = &_game_infos.game_db_write_tps;
			break;
		case dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG:
			info = &_game_infos.game_db_write_msg;
			break;
		case dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS:
			info = &_game_infos.game_auth_fps;
			break;
		case dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS:
			info = &_game_infos.game_server_fps;
			break;
		case dfMONITOR_DATA_TYPE_GAME_PACKET_POOL:
			info = &_game_infos.game_packet_pool;
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
	__forceinline bool CopyInfos(GameServerInfo* game_infos)
	{
		CSLock _lock_guard(&_monitor_cs);
		if (_login_flag == false)
			return false;

		*game_infos = _game_infos;
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
		_game_infos.game_run.Clear();
		_game_infos.game_cpu.Clear();
		_game_infos.game_mem.Clear();
		_game_infos.game_session_num.Clear();
		_game_infos.game_auth_num.Clear();
		_game_infos.game_player_num.Clear();
		_game_infos.game_recv_tps.Clear();
		_game_infos.game_send_tps.Clear();
		_game_infos.game_accept_tps.Clear();
		_game_infos.game_db_write_tps.Clear();
		_game_infos.game_db_write_msg.Clear();
		_game_infos.game_auth_fps.Clear();
		_game_infos.game_server_fps.Clear();
		_game_infos.game_packet_pool.Clear();
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
	GameServerInfo _game_infos;
	uint16 _sector_infos[MAX_SECTOR_SIZE_Y][MAX_SECTOR_SIZE_X];
};


