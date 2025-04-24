#pragma once
#include "pch.h"

struct DataInfo
{
	void Clear()
	{
		total = 0;
		min_val = INT32_MAX;
		max_val = INT32_MIN;
		cnt = 0;
	}
	uint64 total;
	int32 min_val;
	int32 max_val;
	int32 cnt;
};

struct LoginServerInfo
{
	DataInfo login_run;
	DataInfo login_cpu;
	DataInfo login_mem;
	DataInfo login_session_num;
	DataInfo login_auth_tps;
	DataInfo login_packet_pool;
};


struct GameServerInfo
{
	DataInfo game_run;
	DataInfo game_cpu;
	DataInfo game_mem;
	DataInfo game_session_num;
	DataInfo game_auth_num;
	DataInfo game_player_num;
	DataInfo game_recv_tps;
	DataInfo game_send_tps;
	DataInfo game_accept_tps;
	DataInfo game_db_write_tps;
	DataInfo game_db_write_msg;
	DataInfo game_auth_fps;
	DataInfo game_server_fps;
	DataInfo game_packet_pool;

};


struct ChatServerInfo
{
	DataInfo chat_run;
	DataInfo chat_cpu;
	DataInfo chat_mem;
	DataInfo chat_session_num;
	DataInfo chat_player_num;
	DataInfo chat_update_tps;
	DataInfo chat_packet_pool;
	DataInfo chat_msg_pool;
};

struct HardwareInfo
{
	DataInfo hardware_cpu;
	DataInfo hardware_mem;
	DataInfo hardware_recv;
	DataInfo hardware_send;
	DataInfo hardware_available_mem;
	DataInfo hardware_retransmission;
};


enum ServerNo : uint16
{
	NoneServerNo = 0,
	LoginServerNo = 1,
	ChatServerNo = 2,
	GameServerNo = 3,
	HardwareServerNo = 4,
};

 
enum class NetMonitorLogicID
{
	MonitorSoloType = 0,
	MaxServerType,
};

enum class NetMonitorGUILogicID
{
	MonitorGUISoloType = 0,
	MaxServerType,
};