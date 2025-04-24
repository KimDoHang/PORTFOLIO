#include "pch.h"
#include "LanMonitor.h"
#include "MonitorManager.h"
#include "TextParser.h"

void LanMonitor::LanMonitorInit(MonitorManager* manager)
{
	TextParser parser;
	parser.LoadFile(L"LanMonitor.cnf");
	WCHAR ip[50];
	WCHAR log_level[50];
	int32 port;
	int32 concurrent_thread_num;
	int32 max_thread_num;
	int32 max_player_num;
	int32 max_session_num;
	int32 accept_job;
	bool nagle_onoff = true;

	parser.GetString(L"BIND_IP", ip);
	parser.GetValue(L"BIND_PORT", &port);

	parser.GetValue(L"IOCP_WORKER_THREAD", &max_thread_num);
	parser.GetValue(L"IOCP_ACTIVE_THREAD", &concurrent_thread_num);
	parser.GetValue(L"SESSION_MAX", &max_session_num);
	parser.GetValue(L"USER_MAX", &max_player_num);
	parser.GetValue(L"ACCEPT_JOB", &accept_job);
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
	_cur_monitor_server_num = 0;
	_max_monitor_server_num = max_player_num;
	_arr_num = max_session_num;

	_manager = manager;
	_monitor_servers = new ConnectServerNo[_arr_num];

	for (int i = 0; i < _arr_num; i++)
	{
		_monitor_servers[i]._server_no = 0;
		_monitor_servers[i]._hardware_send = 0;
	}

	Init(ip, port, concurrent_thread_num, max_thread_num, true, false, max_session_num, accept_job);

}

bool LanMonitor::OnAccept(uint64 session_id)
{
	if (_cur_monitor_server_num > _max_monitor_server_num)
		return false;

	return true;
}

void LanMonitor::OnRelease(uint64 session_id)
{
	//무조건 먼저 등록하므로 최초 등록이 실패하면 Hardware 등록도 실패한다.
	if (_monitor_servers[static_cast<SHORT>(session_id)]._server_no == 0)
		return;

	WCHAR* log_buff;

	uint16 server_no = _monitor_servers[static_cast<SHORT>(session_id)]._server_no;
	uint8 hardware_flag = _monitor_servers[static_cast<SHORT>(session_id)]._hardware_send;

	switch (server_no)
	{
	case ServerNo::LoginServerNo:
		if (g_monitor_login_server->Clear(session_id) == false)
		{
			log_buff = g_logutils->GetLogBuff(L"Server MisMatch [ServerNo:%d] [Hardware:%d]", (int32)server_no, (int32)hardware_flag);
			g_logutils->Log(LAN_LOG_DIR, L"LanMonitor", log_buff);
			__debugbreak();
		}
		break;
	case ServerNo::ChatServerNo:
		if (g_monitor_chat_server->Clear(session_id) == false)
		{
			log_buff = g_logutils->GetLogBuff(L"Server MisMatch [ServerNo:%d] [Hardware:%d]", (int32)server_no, (int32)hardware_flag);
			g_logutils->Log(LAN_LOG_DIR, L"LanMonitor", log_buff);
			__debugbreak();
		}
		break;
	case ServerNo::GameServerNo:
		if (g_monitor_game_server->Clear(session_id) == false)
		{
			log_buff = g_logutils->GetLogBuff(L"Server MisMatch [ServerNo:%d] [Hardware:%d]", (int32)server_no, (int32)hardware_flag);
			g_logutils->Log(LAN_LOG_DIR, L"LanMonitor", log_buff);
			__debugbreak();
		}
		break;
	default:
	{
		log_buff = g_logutils->GetLogBuff(L"Wrong ServerNo [ServerNo:%d] [Hardware:%d]", (int32)server_no, (int32)hardware_flag);
		g_logutils->Log(LAN_LOG_DIR, L"LanMonitor", log_buff);
		__debugbreak();
	}
	break;
	}

	_monitor_servers[static_cast<SHORT>(session_id)]._server_no = 0;

	if (hardware_flag == 1)
	{
		if (g_monitor_hardware_server->Clear(session_id) == false)
		{
			log_buff = g_logutils->GetLogBuff(L"Wrong HardwareNo [ServerNo:%d] [Hardware:%d]", (int32)server_no, (int32)hardware_flag);
			g_logutils->Log(LAN_LOG_DIR, L"LanMonitor", log_buff);
			__debugbreak();
		}

		_monitor_servers[static_cast<SHORT>(session_id)]._hardware_send = 0;
	}

	log_buff = g_logutils->GetLogBuff(L"Server OFF [ServerNo:%d] [Hardware:%d]", (int32)server_no, (int32)hardware_flag);
	g_logutils->Log(LAN_LOG_DIR, L"LanMonitor", log_buff);

	AtomicDecrement32(&_cur_monitor_server_num);
}

void LanMonitor::OnRecvMsg(uint64 session_id, LanSerializeBuffer* msg)
{
	WORD type = UINT16_MAX;

	try
	{
		*msg >> type;

		switch (type)
		{
		case en_PACKET_SS_MONITOR_LOGIN:
			UnMarshalMonitorServerReqLogin(session_id, msg);
			break;
		case en_PACKET_SS_MONITOR_DATA_UPDATE:
			UnMarshalMonitorServerUpdate(session_id, msg);
			break;
		case en_PACKET_SS_MONITOR_SECTOR_INFO:
			UnMashalMonitorGUIServerUpdate(session_id, msg);
			break;
		default:
			OnError(dfLOG_LEVEL_SYSTEM, df_LAN_PACKET_TYPE_ERROR, L"ServerNo Error");
			Disconnect(session_id);
			return;
		}

	}
	catch (const LanMsgException& lan_msg_exception)
	{
		OnError(dfLOG_LEVEL_SYSTEM, session_id, type, lan_msg_exception.whatW());
		Disconnect(session_id);
	}

}

void LanMonitor::OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause)
{
	WCHAR log_buff[df_LOG_BUFF_SIZE];

	if (g_logutils->GetLogLevel() <= log_level)
	{
		wsprintfW(log_buff, L"%s [ErrCode:%d] [SessionID:%I64u]", cause, err_code, session_id);
		g_logutils->Log(LAN_LOG_DIR, L"LanMonitor", cause);
	}

}


void LanMonitor::OnError(const int8 log_level, int32 err_code, const WCHAR* cause)
{
	WCHAR log_buff[df_LOG_BUFF_SIZE];

	if (g_logutils->GetLogLevel() <= log_level)
	{
		wsprintfW(log_buff, L"%s [ErrCode:%d]", cause, err_code);
		g_logutils->Log(LAN_LOG_DIR, L"LanMonitor", cause);
	}

}

bool LanMonitor::OnConnectionRequest(WCHAR* IP, SHORT port)
{
	return true;
}

bool LanMonitor::OnTimeOut(uint64 session_id)
{
	Disconnect(session_id);
	return true;
}

void LanMonitor::OnMonitor()
{
}

void LanMonitor::OnExit()
{
}



void LanMonitor::UnMarshalMonitorServerReqLogin(uint64 session_id, LanSerializeBuffer* msg)
{
	uint16 server_no;
	uint8 hardware_flag;
	*msg >> server_no >> hardware_flag;

	switch (server_no)
	{
	case ServerNo::LoginServerNo:
		HandleLoginServerReqLogin(session_id, hardware_flag);
		break;
	case ServerNo::ChatServerNo:
		HandlelChatServerReqLogin(session_id, hardware_flag);
		break;
	case ServerNo::GameServerNo:
		HandlelGameServerReqLogin(session_id, hardware_flag);
		break;
	default:
		OnError(dfLOG_LEVEL_SYSTEM, df_NET_SERVER_NO_ERROR, L"ServerNo Error");
		Disconnect(session_id);
		return;
	}

}

void LanMonitor::HandleLoginServerReqLogin(uint64 session_id, uint8 hardware_flag)
{
	if (AtomicIncrement32(&_cur_monitor_server_num) > _max_monitor_server_num)
	{
		AtomicDecrement32(&_cur_monitor_server_num);
		Disconnect(session_id);
		OnError(dfLOG_LEVEL_SYSTEM, 0, L"Max LanMonitor Player num");
		return;
	}

	if(g_monitor_login_server->CheckLogin(session_id) == false)
	{
		Disconnect(session_id);
		OnError(dfLOG_LEVEL_SYSTEM, 0, L"Login Server Already Connected");
		return;
	}

	//이후 삭제시 LoginServer 반환
	_monitor_servers[static_cast<SHORT>(session_id)]._server_no = ServerNo::LoginServerNo;

	if (hardware_flag == 1)
	{
		if (g_monitor_hardware_server->CheckLogin(session_id) == false)
		{
			Disconnect(session_id);
			OnError(dfLOG_LEVEL_SYSTEM, 0, L"Hardware Server Already Connect");
			return;
		}

		_monitor_servers[static_cast<SHORT>(session_id)]._hardware_send = 1;
	}

	//성공 전송
	MarshalMonitorLoginRes(session_id, en_PACKET_SS_MONITOR_LOGIN_RES, 1); 
}

void LanMonitor::HandlelChatServerReqLogin(uint64 session_id, uint8 hardware_flag)
{
	if (AtomicIncrement32(&_cur_monitor_server_num) > _max_monitor_server_num)
	{
		AtomicDecrement32(&_cur_monitor_server_num);
		Disconnect(session_id);
		OnError(dfLOG_LEVEL_SYSTEM, 0, L"Max LanMonitor Player num");
		return;
	}

	if (g_monitor_chat_server->CheckLogin(session_id) == false)
	{
		Disconnect(session_id);
		OnError(dfLOG_LEVEL_SYSTEM, 0, L"Chat Server Already Connect Error");

		return;
	}

	_monitor_servers[static_cast<SHORT>(session_id)]._server_no = ServerNo::ChatServerNo;

	if (hardware_flag == 1)
	{
		if (g_monitor_hardware_server->CheckLogin(session_id) == false)
		{
			Disconnect(session_id);
			OnError(dfLOG_LEVEL_SYSTEM, 0, L"Hardware Server Already Connect Error");
			return;
		}

		_monitor_servers[static_cast<SHORT>(session_id)]._hardware_send = 1;
	}

	MarshalMonitorLoginRes(session_id, en_PACKET_SS_MONITOR_LOGIN_RES, 1);

}

void LanMonitor::HandlelGameServerReqLogin(uint64 session_id, uint8 hardware_flag)
{
	if (AtomicIncrement32(&_cur_monitor_server_num) > _max_monitor_server_num)
	{
		AtomicDecrement32(&_cur_monitor_server_num);
		Disconnect(session_id);
		OnError(dfLOG_LEVEL_SYSTEM, 0, L"Max LanMonitor Player num");
		return;
	}

	if (g_monitor_game_server->CheckLogin(session_id) == false)
	{
		Disconnect(session_id);
		OnError(dfLOG_LEVEL_SYSTEM, 0, L"Game Server Already Connect Error");
		return;
	}

	_monitor_servers[static_cast<SHORT>(session_id)]._server_no = ServerNo::GameServerNo;

	if (hardware_flag == 1)
	{
		if (g_monitor_hardware_server->CheckLogin(session_id) == false)
		{
			Disconnect(session_id);
			OnError(dfLOG_LEVEL_SYSTEM, 0, L"Hardware Server Already Connect Error");
			return;
		}

		_monitor_servers[static_cast<SHORT>(session_id)]._hardware_send = 1;
	}

	MarshalMonitorLoginRes(session_id, en_PACKET_SS_MONITOR_LOGIN_RES, 1);

}

void LanMonitor::UnMarshalMonitorServerUpdate(uint64 session_id, LanSerializeBuffer* msg)
{
	uint8 data_type;
	int32 data_val;
	int32 time_stamp;
	
	*msg >> data_type >> data_val >> time_stamp;
	HandlelMonitorServerUpdate(session_id, data_type, data_val, time_stamp);
}

void LanMonitor::HandlelMonitorServerUpdate(uint64 session_id, uint8 data_type, int32 data_val, int32 time_stamp)
{
	uint8 server_no = CheckType(session_id, data_type, data_val, time_stamp);
	
	if (server_no == 0)
	{
		WCHAR* log_buff = g_logutils->GetLogBuff(L"Wrong Data Type: %hhu | Cur ServerNo: %hu | Cur Hardware Send: %d", data_type, _monitor_servers[static_cast<uint16>(session_id)]._server_no, _monitor_servers[static_cast<uint16>(session_id)]._hardware_send);
		OnError(dfLOG_LEVEL_SYSTEM, 0, log_buff);
		Disconnect(session_id);
		return;
	}

	_manager->BroadCastUpdateData(en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE, server_no, data_type, data_val, time_stamp);
}

void LanMonitor::UnMashalMonitorGUIServerUpdate(uint64 session_id, LanSerializeBuffer* msg)
{
	uint8 server_no;
	int32 time_stamp;

	uint16 sector_infos[MAX_SECTOR_SIZE_Y][MAX_SECTOR_SIZE_X];
	
	uint16 player_num;
	*msg >> server_no >> time_stamp;

	for (int y = 0; y < MAX_SECTOR_SIZE_Y; y++)
	{
		for (int x = 0; x < MAX_SECTOR_SIZE_X; x++)
		{
			*msg >> player_num;
			sector_infos[y][x] = player_num;
		}
	}

	HandleMonitorGUIServerUpdate(session_id, server_no, time_stamp, sector_infos);
}

void LanMonitor::HandleMonitorGUIServerUpdate(uint64 session_id, uint8 server_no, int32 time_stamp, uint16 sector_infos[][MAX_SECTOR_SIZE_X])
{ 
	bool ret = false;

	switch (server_no)
	{
	case ChatServerNo:
	{
		ChatMonitorServer* server = g_monitor_chat_server;
		ret = server->UpdateSector(session_id, server_no, sector_infos);
	}
		break;
	case GameServerNo:
	{
		GameMonitorServer* server = g_monitor_game_server;
		ret = server->UpdateSector(session_id, server_no, sector_infos);
	}
		break;
	default:
		OnError(dfLOG_LEVEL_SYSTEM, session_id,  df_NET_SERVER_NO_ERROR, L"ServerNo Error");
		Disconnect(session_id);
		return;
	}

	if (ret == false)
	{
		OnError(dfLOG_LEVEL_SYSTEM, session_id, df_NET_SERVER_NO_ERROR, L"MonitorServer ID or Type Error");
		Disconnect(session_id);
		return;
	}

	_manager->BroadCastSectorData(en_PACKET_SC_MONITOR_DISTRIBUTION_SECTOR_INFO, server_no, time_stamp, sector_infos);
}

void LanMonitor::MarshalMonitorLoginRes(uint64 session_id, uint16 type, uint8 status)
{
	LanSerializeBuffer* msg = ALLOC_LAN_PACKET();
	*msg << type << status;
	SendPacket(session_id, msg);
	FREE_LAN_SEND_PACKET(msg);
}


uint8 LanMonitor::CheckType(uint64 session_id, uint8 data_type, int32 data_val, int32 time_stamp)
{
	MonitorServer* monitor_server;
	uint8 server_no;

	switch (data_type)
	{
	case dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN:
	case dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU:
	case dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM:
	case dfMONITOR_DATA_TYPE_LOGIN_SESSION:
	case dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS:
	case dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL:
	{
		monitor_server = reinterpret_cast<MonitorServer*>(g_monitor_login_server);
		break;
	}
	case dfMONITOR_DATA_TYPE_GAME_SERVER_RUN:
	case dfMONITOR_DATA_TYPE_GAME_SERVER_CPU:
	case dfMONITOR_DATA_TYPE_GAME_SERVER_MEM:
	case dfMONITOR_DATA_TYPE_GAME_SESSION:
	case dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER:
	case dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER:
	case dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS:
	case dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS:
	case dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS:
	case dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS:
	case dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG:
	case dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS:
	case dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS:
	case dfMONITOR_DATA_TYPE_GAME_PACKET_POOL:
	{
		monitor_server = reinterpret_cast<MonitorServer*>(g_monitor_game_server);
		break;
	}
	case dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN:
	case dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU:
	case dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM:
	case dfMONITOR_DATA_TYPE_CHAT_SESSION:
	case dfMONITOR_DATA_TYPE_CHAT_PLAYER:
	case dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS:
	case dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL:
	case dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL:
	{
		monitor_server = reinterpret_cast<MonitorServer*>(g_monitor_chat_server);
		break;
	}
	case dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL:
	case dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY:
	case dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV:
	case dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND:
	case dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY:
	case dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RETRANSMISSION:
	{
		monitor_server = reinterpret_cast<MonitorServer*>(g_monitor_hardware_server);
		break;
	}
	default:
		return 0;
	}

	if (monitor_server->UpdateData(data_type, data_val, time_stamp) == false)
	{
		return 0;
	}

	//이때 해당 Session의 RecvProc에서 처리되는 함수들이므로 Session이 반드시 보장된다.
	//따라서 Session에 대한 삭제를 고려할 필요가 없다.
	server_no = monitor_server->GetServerNo();

	if (server_no == 4)
	{
		if (_monitor_servers[static_cast<uint16>(session_id)]._hardware_send == 0)
			return 0;
	}
	else if(_monitor_servers[static_cast<uint16>(session_id)]._server_no != server_no)
	{
		return 0;
	}


	return server_no;
}
