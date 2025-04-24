#include "pch.h"
#include "MonitorGUIServer.h"
#include "MonitorManager.h"
#include "TextParser.h"
#include "GUISolo.h"

void MonitorGUIServer::MonitrGUIInit(MonitorManager* manager)
{
	TextParser parser;
	parser.LoadFile(df_MONITORGUI_FILE_NAME);

	WCHAR ip[50];
	WCHAR log_level[50];
	int32 port;
	int32 concurrent_thread_num;
	int32 max_thread_num;
	int32 max_session_num;
	int32 max_player_num;
	int32 packet_code;
	int32 packet_key;
	int32 accept_job;

	bool nagle_onoff = true;

	parser.GetString(L"BIND_IP", ip);
	parser.GetValue(L"BIND_PORT", &port);

	parser.GetValue(L"IOCP_WORKER_THREAD", &max_thread_num);
	parser.GetValue(L"IOCP_ACTIVE_THREAD", &concurrent_thread_num);
	parser.GetValue(L"SESSION_MAX", &max_session_num);
	parser.GetValue(L"USER_MAX", &max_player_num);

	parser.GetValue(L"ACCEPT_JOB", &accept_job);
	parser.GetValue(L"PACKET_CODE", &packet_code);
	parser.GetValue(L"PACKET_KEY", &packet_key);
	parser.GetString(L"LOG_LEVEL", log_level);

	if (wcscmp(log_level, L"DEBUG") == 0)
	{
		g_logutils->SetLogLevel(dfLOG_LEVEL_DEBUG);
	}
	else if (wcscmp(log_level, L"ERROR") == 0)
	{
		g_logutils->SetLogLevel(dfLOG_LEVEL_ERROR);
	}
	else
	{
		g_logutils->SetLogLevel(dfLOG_LEVEL_SYSTEM);
	}

	_manager = manager;
	_max_monitor_gui_num = max_player_num;
	_cur_monitor_gui_num = 0;
	_arr_num = max_session_num;

	_monitor_gui_arr = new MonitorGUI[_arr_num];

	for (int idx = 0; idx < _arr_num; idx++)
	{
		_monitor_gui_arr[idx].session_id = UINT16_MAX;
		_monitor_gui_arr[idx].login_flag = false;
		_monitor_gui_arr[idx].server_no = 0;
	}

	Init(ip, port, concurrent_thread_num, max_thread_num, packet_code, packet_key, true, false, max_session_num, accept_job);

	_gui_solo = new GUISolo(this, (uint16)NetMonitorGUILogicID::MonitorGUISoloType);

	CreateLogicInstanceArr((int)NetMonitorGUILogicID::MaxServerType);
	AttachLogicInstance((int)NetMonitorGUILogicID::MonitorGUISoloType, reinterpret_cast<LogicInstance*>(_gui_solo));
}

bool MonitorGUIServer::OnAccept(uint64 session_id)
{
	_gui_solo->RegisterAccept(session_id);

	if (_cur_monitor_gui_num > _max_monitor_gui_num)
		return false;

	return true;
}

void MonitorGUIServer::OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause)
{
	WCHAR log_buff[df_LOG_BUFF_SIZE];

	if (g_logutils->GetLogLevel() <= log_level)
	{
		wsprintfW(log_buff, L"%s [ErrCode:%d] [SessionID:%I64u]", cause, err_code, session_id);
		g_logutils->Log(LAN_LOG_DIR, df_MONITORGUI_LOG_FILE_NAME, cause);
	}
}

void MonitorGUIServer::OnError(const int8 log_level, int32 err_code, const WCHAR* cause)
{
	WCHAR log_buff[df_LOG_BUFF_SIZE];

	if (g_logutils->GetLogLevel() <= log_level)
	{
		wsprintfW(log_buff, L"%s [ErrCode:%d]", cause, err_code);
		g_logutils->Log(NET_LOG_DIR, df_MONITORGUI_LOG_FILE_NAME, cause);
	}
}

bool MonitorGUIServer::OnConnectionRequest(WCHAR* IP, SHORT port)
{
	return true;
}

bool MonitorGUIServer::OnTimeOut(uint64 session_id)
{
	Disconnect(session_id);
	return true;
}

void MonitorGUIServer::OnMonitor()
{



}


void MonitorGUIServer::OnExit()
{

}


void MonitorGUIServer::SendAllMonitorInfoMsg(uint16 type, uint8 server_no, int32 time_stamp, uint16 sector_infos[][MAX_SECTOR_SIZE_X])
{
	NetSerializeBuffer* msg = ALLOC_NET_PACKET();

	*msg << type << time_stamp;

	for (int y = 0; y < MAX_SECTOR_SIZE_Y; y++)
	{
		for (int x = 0; x < MAX_SECTOR_SIZE_X; x++)
		{
			*msg << sector_infos[y][x];
		}
	}

	for (int i = 0; i < _arr_num; i++)
	{
		//해당 Session ID라면 초기화 X
		if (_monitor_gui_arr[i].session_id == UINT16_MAX)
			continue;

		// id 변경 후 login flag 변경하므로 직렬 처리되거나 실패한다.
		if (_monitor_gui_arr[i].login_flag && _monitor_gui_arr[i].server_no == server_no)
		{
			SendPacket(_monitor_gui_arr[i].session_id, msg);
		}
	}

	FREE_NET_SEND_PACKET(msg);
}

char* MonitorGUIServer::GetLoginKey()
{
	return _manager->_login_key;
}
