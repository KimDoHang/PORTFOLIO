#pragma once
//Send Tick 

#include "LoginMonitorServer.h"
#include "ChatMonitorServer.h"
#include "GameMonitorServer.h"
#include "HardwareMonitorServer.h"
#include "SqlUtils.h" 
#include "NetMonitor.h"
#include "LanMonitor.h"
#include "MonitorGUIServer.h"

extern HardwareMonitorServer* g_monitor_hardware_server;
extern LoginMonitorServer* g_monitor_login_server;
extern ChatMonitorServer* g_monitor_chat_server;
extern GameMonitorServer* g_monitor_game_server;
extern SqlUtils* g_sql_manager;

class MonitorManager
{	
	friend class LanMonitor;
	friend class NetMonitor;
	friend class MonitorGUIServer;

	static unsigned MonitorDBThread(void* manager);

public:
	
	
	~MonitorManager();

	void MonitorInit();
	void MonitorStart();
	void BroadCastUpdateData(uint16 type, uint8 server_no, uint8 data_type, int32 data_val, int32 time_stamp);
	void BroadCastSectorData(uint16 type, uint8 server_no, int32 time_stamp, uint16 sector_infos[][MAX_SECTOR_SIZE_X]);

	void InitDB();
	void DBThreadLoop();


	void InsertHardwareQuery(const tm& cur_time, const uint8 server_no, HardwareInfo* hardware_info, MYSQL* connector);
	void InsertLoginQuery(const tm& cur_time, const uint8 server_no, LoginServerInfo* login_info, MYSQL* connector);
	void InsertChatQuery(const tm& cur_time, const uint8 server_no, ChatServerInfo* chat_info, MYSQL* connector);
	void InsertGameQuery(const tm& cur_time, const uint8 server_no, GameServerInfo* game_info, MYSQL* connector);

	__forceinline void InsertQuery(const tm& cur_time, const uint8 server_no, const uint8 data_type, DataInfo* info, MYSQL* connector)
	{
		char monitor_db_table[df_QUERY_BUFF_SIZE];
		sprintf_s(monitor_db_table, df_QUERY_BUFF_SIZE, "monitorlog_%d%d", cur_time.tm_year + 1900, cur_time.tm_mon + 1);

		char query[df_QUERY_BUFF_SIZE];

		if (info->cnt != 0)
		{
			sprintf_s(query, df_QUERY_BUFF_SIZE, "insert into %s (logtime, serverno, type, avg, min, max, tick) VALUES (now(), %d, %d, %d, %d, %d, %d)", monitor_db_table, (int)server_no, (int)data_type, (int)(info->total / info->cnt), (int)info->min_val, (int)info->max_val, (int)info->cnt);
		}
		else
		{
			sprintf_s(query, df_QUERY_BUFF_SIZE, "insert into %s (logtime, serverno, type, avg, min, max, tick) VALUES (now(), %d, %d, %d, %d, %d, %d)", monitor_db_table, (int)server_no, (int)data_type, 0, 0, 0, (int)info->cnt);
		}


		int query_ret = g_sql_manager->SendQuery(connector, query);

		if (query_ret != 0)
		{
			if (query_ret == df_TABLE_NOT_EXIST_SQL_ERROR)
			{
				// Table을 새로 생성하고 다시 Query를 실행한다.
				char create_table_query[df_QUERY_BUFF_SIZE];
				sprintf_s(create_table_query, df_QUERY_BUFF_SIZE, "create table %s like monitorlog", monitor_db_table);

				g_sql_manager->SendQuery(connector, create_table_query);
				g_sql_manager->SendQuery(connector, query);
			}
			else
			{
				_net_monitor->OnError(dfLOG_LEVEL_SYSTEM, query_ret, L"MY SQL Send Query Fail");
				__debugbreak();
			}
		}
	}


private:
	LanMonitor*				_lan_monitor;
	NetMonitor*				_net_monitor;
	MonitorGUIServer*		_gui_monitor;
	HANDLE _db_thread;
	HANDLE _db_event;
	char _login_key[df_LOGIN_KEY_SIZE];
};





