#include "pch.h"
#include "MonitorClient.h"
#include "LanSerializeBuffer.h"
#include "LanSendPacket.h"
#include "LanRecvPacket.h"

void MonitorClient::SendLoginPacket(uint16 type, int16 server_no, uint8 hardware_flag)
{
	LanSerializeBuffer* msg = ALLOC_LAN_PACKET();
	*msg << type << server_no << hardware_flag;
	SendPacket(msg);
	FREE_LAN_SEND_PACKET(msg);
}

void MonitorClient::SendMontiorPacket(uint16 type, uint8 data_type, int32 data_val, int32 time_stamp)
{
	LanSerializeBuffer* msg = ALLOC_LAN_PACKET();
	*msg << type << data_type << data_val << time_stamp;
	SendPush(msg);
	FREE_LAN_SEND_PACKET(msg);
}

void MonitorClient::MarshalReqSectorInfo(uint16 type, uint8 server_no, int32 time_stamp, uint16 sector_infos[][MAX_SECTOR_SIZE_X])
{
	LanSerializeBuffer* msg = ALLOC_LAN_PACKET();

	*msg << type << server_no << time_stamp;

	for (int y = 0; y < MAX_SECTOR_SIZE_Y; y++)
	{
		for (int x = 0; x < MAX_SECTOR_SIZE_X; x++)
		{
			*msg << sector_infos[y][x];
		}
	}

	SendPacket(msg);
	FREE_LAN_SEND_PACKET(msg);
}

void MonitorClient::OnConnect()
{
	SendLoginPacket(en_PACKET_SS_MONITOR_LOGIN, ServerNo::ChatServerNo, 1);
}

void MonitorClient::OnRelease()
{
	_connect_flag = false;
	OnError(dfLOG_LEVEL_SYSTEM, 0, L"ChatLoignServer LanClient Exit");
}

void MonitorClient::OnRecvMsg(LanSerializeBuffer* packet)
{
	uint16 type = UINT16_MAX;

	try
	{
		*packet >> type;
		switch (type)
		{
		case en_PACKET_SS_MONITOR_LOGIN_RES:
			UnMarshalMonitorResLogin(packet);
			break;
		default:
			OnError(dfLOG_LEVEL_SYSTEM, 0, L"ChatLoignServer LanClient RecvMsg Type Error");
			Disconnect();
			break;
		}

	}
	catch (const LanMsgException& lan_msg_exception)
	{
		OnError(dfLOG_LEVEL_SYSTEM, type, lan_msg_exception.whatW());
		Disconnect();
	}
}

void MonitorClient::OnError(const int8 log_level, int32 err_code, const WCHAR* cause)
{
	if (g_logutils->GetLogLevel() <= log_level)
	{
		g_logutils->Log(NET_LOG_DIR, L"ChatLogin_MonitorClient", cause);
	}
}

void MonitorClient::OnExit()
{

}
