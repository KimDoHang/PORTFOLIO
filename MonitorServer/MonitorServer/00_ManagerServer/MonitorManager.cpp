#include "pch.h"
#include "MonitorManager.h"
#include "TextParser.h"
#include "ThreadManager.h"
#include "MonitorGUIServer.h"


SqlUtils* g_sql_manager = nullptr;

HardwareMonitorServer* g_monitor_hardware_server = nullptr;
LoginMonitorServer* g_monitor_login_server = nullptr;
ChatMonitorServer* g_monitor_chat_server = nullptr;
GameMonitorServer * g_monitor_game_server = nullptr;


unsigned MonitorManager::MonitorDBThread(void* manager)
{
	reinterpret_cast<MonitorManager*>(manager)->DBThreadLoop();
	return 0;
}

MonitorManager::~MonitorManager()
{
	delete g_sql_manager;
	delete g_monitor_login_server;
	delete g_monitor_chat_server;
	delete g_monitor_game_server;
	delete g_monitor_hardware_server;
	delete _net_monitor;
	delete _lan_monitor;
	delete _gui_monitor;
}

void MonitorManager::MonitorInit()
{
	g_sql_manager = new SqlUtils;
	g_monitor_login_server = new LoginMonitorServer;
	g_monitor_chat_server = new ChatMonitorServer;
	g_monitor_game_server = new GameMonitorServer;
	g_monitor_hardware_server = new HardwareMonitorServer;
	_net_monitor = new NetMonitor;
	_gui_monitor = new MonitorGUIServer;
	_lan_monitor = new LanMonitor;

	memcpy(_login_key, "ajfw@!cv980dSZ[fje#@fdj123948djf", df_LOGIN_KEY_SIZE);
	_db_event = CreateEventW(nullptr, true, false, nullptr);

	_lan_monitor->LanMonitorInit(this);
	_net_monitor->NetMonitorInit(this);
	_gui_monitor->MonitrGUIInit(this);

	InitDB();
	_net_monitor->ConsoleSize(25, 100);
}

void MonitorManager::MonitorStart()
{
	SetEvent(_db_event);

	_lan_monitor->Start();
	_net_monitor->Start();
	_gui_monitor->Start();
}

void MonitorManager::BroadCastUpdateData(uint16 type, uint8 server_no, uint8 data_type, int32 data_val, int32 time_stamp)
{
	_net_monitor->SendAllUpdateData(type, server_no, data_type, data_val, time_stamp);
}

void MonitorManager::BroadCastSectorData(uint16 type, uint8 server_no, int32 time_stamp, uint16 sector_infos[][MAX_SECTOR_SIZE_X])
{
	_gui_monitor->SendAllMonitorInfoMsg(type, server_no, time_stamp, sector_infos);
}

void MonitorManager::InitDB()
{
	_db_thread = (HANDLE)_beginthreadex(nullptr, 0, MonitorManager::MonitorDBThread, this, 0, nullptr);
}

void MonitorManager::DBThreadLoop()
{
	g_sql_manager->DBConnect();
	MYSQL* connector = g_sql_manager->GetConnection();

	WaitForSingleObject(_db_event, INFINITE);

	uint32 cur_tick = timeGetTime();
	uint32 next_tick = cur_tick;
	uint32 sleep_tick = 0;
	uint32 db_tick = cur_tick;
	uint32 last_console_time = cur_tick;
	uint32 cur_console_time = cur_tick;

	g_sql_manager->SendQuery(connector, "use `logdb`");

	while (true)
	{
		cur_console_time = cur_tick = timeGetTime();

		if(cur_tick >= db_tick)
		{
			LoginServerInfo login_info;
			ChatServerInfo chat_info; 
			GameServerInfo game_info;
			HardwareInfo hardware_info;
			time_t cur_time = std::time(nullptr);
			tm cur_local_time;
			localtime_s(&cur_local_time, &cur_time);

			bool login_server_flag = g_monitor_login_server->CopyInfos(&login_info);
			bool chat_server_flag = g_monitor_chat_server->CopyInfos(&chat_info);
			bool game_server_flag = g_monitor_game_server->CopyInfos(&game_info);
			bool hardware_server_flag = g_monitor_hardware_server->CopyInfos(&hardware_info);


			if (login_server_flag)
			{
				InsertLoginQuery(cur_local_time, ServerNo::LoginServerNo, &login_info, connector);
			}

			if (chat_server_flag)
			{
				InsertChatQuery(cur_local_time, ServerNo::ChatServerNo, &chat_info, connector);
			}

			if (game_server_flag)
			{
				InsertGameQuery(cur_local_time, ServerNo::GameServerNo, &game_info, connector);
			}

			if (hardware_server_flag)
			{
				InsertHardwareQuery(cur_local_time, ServerNo::HardwareServerNo, &hardware_info, connector);
			}

			db_tick += df_DB_TICK;
		}

		_lan_monitor->PrintMonitor();
		_net_monitor->PrintMonitor();
		_gui_monitor->PrintMonitor();

		printf("Console: %d\n", cur_console_time - last_console_time);
		last_console_time = cur_console_time;

		sleep_tick = 0;

		next_tick += df_FRAME_TICK;

		cur_tick = timeGetTime();

		if (next_tick > cur_tick)
			sleep_tick = next_tick - cur_tick;

		Sleep(sleep_tick);
	}

}

void MonitorManager::InsertLoginQuery(const tm& cur_time, const uint8 server_no, LoginServerInfo* login_info, MYSQL* connector)
{
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN, &login_info->login_run, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU, &login_info->login_cpu, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM, &login_info->login_mem, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_LOGIN_SESSION, &login_info->login_session_num, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS, &login_info->login_auth_tps, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL, &login_info->login_packet_pool, connector);
}

void MonitorManager::InsertChatQuery(const tm& cur_time, const uint8 server_no, ChatServerInfo* chat_info, MYSQL* connector)
{
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, &chat_info->chat_run, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, &chat_info->chat_cpu, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, &chat_info->chat_mem, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_CHAT_SESSION, &chat_info->chat_session_num, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_CHAT_PLAYER, &chat_info->chat_player_num, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, &chat_info->chat_update_tps, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, &chat_info->chat_packet_pool, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, &chat_info->chat_msg_pool, connector);
}

void MonitorManager::InsertGameQuery(const tm& cur_time, const uint8 server_no, GameServerInfo* game_info, MYSQL* connector)
{
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_GAME_SERVER_RUN, &game_info->game_run, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_GAME_SERVER_CPU, &game_info->game_cpu, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_GAME_SERVER_MEM, &game_info->game_mem, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_GAME_SESSION, &game_info->game_session_num, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER, &game_info->game_auth_num, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER, &game_info->game_player_num, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS, &game_info->game_accept_tps, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS, &game_info->game_recv_tps, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS, &game_info->game_send_tps, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS, &game_info->game_db_write_tps, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG, &game_info->game_db_write_msg, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS, &game_info->game_auth_fps, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS, &game_info->game_server_fps, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_GAME_PACKET_POOL,&game_info->game_packet_pool, connector);
}

void MonitorManager::InsertHardwareQuery(const tm& cur_time, const uint8 server_no, HardwareInfo* hardware_info, MYSQL* connector)
{

	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, &hardware_info->hardware_cpu, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, &hardware_info->hardware_mem, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, &hardware_info->hardware_recv, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, &hardware_info->hardware_send, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, &hardware_info->hardware_available_mem, connector);
	InsertQuery(cur_time, server_no, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RETRANSMISSION, &hardware_info->hardware_retransmission, connector);

}
