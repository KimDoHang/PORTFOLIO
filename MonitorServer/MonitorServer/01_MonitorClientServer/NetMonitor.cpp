#include "pch.h"
#include "NetMonitor.h"
#include "LogUtils.h"
#include "MonitorManager.h"
#include "Lock.h"
#include "ChatMonitorServer.h"
#include "GameMonitorServer.h"
#include "LoginMonitorServer.h"
#include "MonitorSolo.h"
#include "TextParser.h"

void NetMonitor::NetMonitorInit(MonitorManager* manager)
{
	TextParser parser;
	parser.LoadFile(L"NetMonitor.cnf");

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
	_max_tool_num = max_player_num;
	_cur_tool_num = 0;
	_monitor_tool_client_arr = new MonitorTool[_max_tool_num];
	_arr_num = max_session_num;

	for (int idx = 0; idx < _arr_num; idx++)
	{
		_monitor_tool_client_arr[idx].session_id = UINT16_MAX;
		_monitor_tool_client_arr[idx].login_flag = false;
	}

	_monitor_solo = new MonitorSolo(this, (int)NetMonitorLogicID::MonitorSoloType);
	CreateLogicInstanceArr((int)NetMonitorLogicID::MaxServerType);
	AttachLogicInstance((int)NetMonitorLogicID::MonitorSoloType, reinterpret_cast<LogicInstance*>(_monitor_solo));

	Init(ip, port, concurrent_thread_num, max_thread_num, packet_code, packet_key, true, false, max_session_num, accept_job);
}

bool NetMonitor::OnAccept(uint64 session_id)
{
	_monitor_solo->RegisterAccept(session_id);

	if (_cur_tool_num > _max_tool_num)
		return false;

	return true;
}

void NetMonitor::OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause)
{
	WCHAR log_buff[df_LOG_BUFF_SIZE];

	if (g_logutils->GetLogLevel() <= log_level)
	{
		wsprintfW(log_buff, L"%s [ErrCode:%d] [SessionID:%I64u]", cause, err_code, session_id);
		g_logutils->Log(NET_LOG_DIR, L"NetMonitor", cause);
	}

}

void NetMonitor::OnError(const int8 log_level, int32 err_code, const WCHAR* cause)
{
	WCHAR log_buff[df_LOG_BUFF_SIZE];

	if (g_logutils->GetLogLevel() <= log_level)
	{
		wsprintfW(log_buff, L"%s [ErrCode:%d]", cause, err_code);
		g_logutils->Log(NET_LOG_DIR, L"NetMonitor", cause);
	}
}

bool NetMonitor::OnConnectionRequest(WCHAR* IP, SHORT port)
{
	return true;
}

bool NetMonitor::OnTimeOut(uint64 thread_id)
{
	return true;
}

void NetMonitor::OnMonitor()
{


}

void NetMonitor::OnExit()
{


}

void NetMonitor::SendAllUpdateData(uint16 type, uint8 server_no, uint8 data_type, int32 data_val, int32 time_stamp)
{
	NetSerializeBuffer* msg = ALLOC_NET_PACKET();
	*msg << type << server_no << data_type << data_val << time_stamp;
	
	for (int i = 0; i < _arr_num; i++)
	{

		// 다른 스레드에서 처리됨
		//하지만 session_id가 비순차 실행으로 읽어 오더라도 반드시 로그인 처리가 끝난 후 변경이 이루어지므로 
		//login_flag를 과거를 읽어 통과하더라도 session_id는 과거값이거나 현재 값인게 보장된다.
		//따라서 반드시 받지는 못하지만 로그인 처리가 이루어지기 전에 받는 것은 불가능하다.
		if (_monitor_tool_client_arr[i].login_flag)
		{
			SendPacket(_monitor_tool_client_arr[i].session_id, msg);
		}
	}

	FREE_NET_SEND_PACKET(msg);
}

char* NetMonitor::GetLoginKey()
{
	return _manager->_login_key;
}