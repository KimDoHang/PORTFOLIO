#include "pch.h"
#include "GUISolo.h"
#include "MonitorGUIServer.h"

void GUISolo::OnEnter(uint64 session_id, SessionInstance* player)
{
}

void GUISolo::OnLeave(uint64 session_id, SessionInstance* player)
{
}

void GUISolo::OnRelease(uint64 session_id, SessionInstance* player)
{
	MonitorGUIServer* server = reinterpret_cast<MonitorGUIServer*>(_service);

	if (server->_monitor_gui_arr[static_cast<uint16>(session_id)].login_flag == false)
		return;

	server->OnError(dfLOG_LEVEL_SYSTEM, session_id, 0, L"MonitorGUI Client Exit");

	//로그인 처리에 대한 패킷 처리와 Release Packet 처리는 동시에 불가능 -> OnRecv에서 처리하므로 반드시 직렬적으로 처리
	server->_monitor_gui_arr[static_cast<uint16>(session_id)].session_id = UINT16_MAX;
	server->_monitor_gui_arr[static_cast<uint16>(session_id)].login_flag = false;
	server->_monitor_gui_arr[static_cast<uint16>(session_id)].server_no = 0;
	AtomicDecrement32(&server->_cur_monitor_gui_num);
}

bool GUISolo::OnRecvMsg(uint64 session_id, SessionInstance* player, NetSerializeBuffer* msg)
{
	WORD type = UINT16_MAX;

	try
	{
		*msg >> type;

		switch (type)
		{
		case en_PACKET_CS_MONITOR_DISTRIBUTION_REQ_LOGIN:
			UnMarshaMonitorGUILogin(session_id, msg);
			break;
		case en_PACKET_CS_GUI_REQ_HEARTBEAT:
			HandleMonitorGUIHeartBeatReq(session_id);
			break;
		default:
			_service->OnError(dfLOG_LEVEL_SYSTEM, df_NET_PACKET_TYPE_ERROR, L"MonitrGUIClient Packet Type Error");
			Disconnect(session_id);
			break;
		}

	}
	catch (const NetMsgException& net_msg_exception)
	{
		_service->OnError(dfLOG_LEVEL_SYSTEM, session_id, type, net_msg_exception.whatW());
		Disconnect(session_id);
	}	

	return true;
}

void GUISolo::UnMarshaMonitorGUILogin(uint64 session_id, NetSerializeBuffer* msg)
{
	uint8 server_no;
	char login_key[32];

	*msg >> server_no;
	msg->GetData(reinterpret_cast<char*>(login_key), df_LOGIN_KEY_SIZE);

	HandleMonitorGUILogin(session_id, server_no, login_key);
}

void GUISolo::HandleMonitorGUILogin(uint64 session_id, uint8 server_no, char* login_key)
{
	bool status = true;

	MonitorGUIServer* server = reinterpret_cast<MonitorGUIServer*>(_service);

	do
	{
		if (AtomicIncrement32(&server->_cur_monitor_gui_num) > server->_max_monitor_gui_num)
		{
			AtomicDecrement32(&server->_cur_monitor_gui_num);
			status = false;
			server->OnError(dfLOG_LEVEL_SYSTEM, session_id, 0, L"Max Monitor GUI");
			break;
		}

		if (strncmp(server->GetLoginKey(), login_key, df_LOGIN_KEY_SIZE) != 0)
		{
			//LOGIN KEY NOT CORRECT
			status = false;
			server->OnError(dfLOG_LEVEL_SYSTEM, session_id, 0, L"Login Key Not Correct");
			break;
		}

	} while (false);


	if (status == false)
	{
		//성공실패시 SendDisconnect으로 처리
		Disconnect(session_id);
		return;
	}

	MarshalResMonitorLogin(session_id, en_PACKET_SC_MONITOR_DISTRIBUTION_RES_LOGIN, MAX_SECTOR_SIZE_X, MAX_SECTOR_SIZE_Y, status);

	server->_monitor_gui_arr[static_cast<uint16>(session_id)].server_no = server_no;
	server->_monitor_gui_arr[static_cast<uint16>(session_id)].login_flag = true;
	server->_monitor_gui_arr[static_cast<uint16>(session_id)].session_id = session_id;
}

void GUISolo::HandleMonitorGUIHeartBeatReq(uint64 session_id)
{
	return;
}

void GUISolo::MarshalResMonitorLogin(uint64 session_id, uint16 type, uint16 sector_x_size, uint16 sector_y_size, BYTE status)
{
	NetSerializeBuffer* msg = ALLOC_NET_PACKET();
	*msg << type << sector_x_size << sector_y_size << status;

	_service->SendPacket(session_id, msg);
	FREE_NET_SEND_PACKET(msg);
}
