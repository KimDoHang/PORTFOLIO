#include "pch.h"
#include "MonitorSolo.h"

void MonitorSolo::OnEnter(uint64 session_id, SessionInstance* player)
{
}

void MonitorSolo::OnLeave(uint64 session_id, SessionInstance* player)
{

}

void MonitorSolo::OnRelease(uint64 session_id, SessionInstance* player)
{
	NetMonitor* server = reinterpret_cast<NetMonitor*>(_service);

	if (server->_monitor_tool_client_arr[static_cast<uint16>(session_id)].login_flag == false)
		return;

	//�α��� ó���� ���� ��Ŷ ó���� Release Packet ó���� ���ÿ� �Ұ��� -> OnRecv���� ó���ϹǷ� �ݵ�� ���������� ó��
	server->_monitor_tool_client_arr[static_cast<uint16>(session_id)].session_id = UINT16_MAX;
	server->_monitor_tool_client_arr[static_cast<uint16>(session_id)].login_flag = false;

	AtomicDecrement32(&server->_cur_tool_num);
}

bool MonitorSolo::OnRecvMsg(uint64 session_id, SessionInstance* player, NetSerializeBuffer* msg)
{
	WORD type = UINT16_MAX;

	try
	{
		*msg >> type;

		switch (type)
		{
		case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
			UnMarshalMonitorToolReqLogin(session_id, msg);
			break;
		default:
			_service->OnError(dfLOG_LEVEL_SYSTEM, df_NET_PACKET_TYPE_ERROR, L"NetMonitor Packet Type Error");
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

void MonitorSolo::UnMarshalMonitorToolReqLogin(uint64 session_id, NetSerializeBuffer* msg)
{
	char login_key[32];
	msg->GetData(reinterpret_cast<char*>(login_key), df_LOGIN_KEY_SIZE);

	HandlelMonitorToolReqLogin(session_id, login_key);
}

void MonitorSolo::HandlelMonitorToolReqLogin(uint64 session_id, char* login_key)
{
	uint8 status = dfMONITOR_TOOL_LOGIN_OK;

	NetMonitor* server = reinterpret_cast<NetMonitor*>(_service);
	do
	{
		if (AtomicIncrement32(&server->_cur_tool_num) > server->_max_tool_num)
		{
			AtomicDecrement32(&server->_cur_tool_num);

			//MAX STATUS
			status = 3;
			_service->OnError(dfLOG_LEVEL_SYSTEM, session_id, 0, L"MAX Monitor Session Login");
			break;
		}

		if (strncmp(server->GetLoginKey(), login_key, df_LOGIN_KEY_SIZE) != 0)
		{
			//LOGIN KEY NOT CORRECT
			status = 3;
			_service->OnError(dfLOG_LEVEL_SYSTEM, session_id, 0, L"Login Key Not Correct");
			break;
		}

	} while (false);


	if (status != 1)
	{
		//�������н� SendDisconnect���� ó��
		MarshalMonitorToolResLoginDisconnect(session_id, en_PACKET_CS_MONITOR_TOOL_RES_LOGIN, status);
		return;
	}


	//���� Enqueue, IO_CNT�� ���� ����
	MarshalMonitorToolResLogin(session_id, en_PACKET_CS_MONITOR_TOOL_RES_LOGIN, status);
	MonitorTool* monitor_tool = &server->_monitor_tool_client_arr[static_cast<uint16>(session_id)];

	{
		//Store�� ������ ������� ���� (����� �����̴���)
		//���� �α��� ������ ���޵ǰ� ���� session_id�� write�ȴ�. �̶� OnRecv�̹Ƿ� ���ÿ� ó���� �ȵȴ�. -> Lock �ʿ� X

		monitor_tool->session_id = session_id;
		monitor_tool->login_flag = true;
	}


}

void MonitorSolo::MarshalMonitorToolResLogin(uint64 session_id, uint16 type, uint8 status)
{
	NetSerializeBuffer* msg = ALLOC_NET_PACKET();
	*msg << type << status;

	_service->SendPacket(session_id, msg);
	FREE_NET_SEND_PACKET(msg);
}

void MonitorSolo::MarshalMonitorToolResLoginDisconnect(uint64 session_id, uint16 type, uint8 status)
{
	NetSerializeBuffer* msg = ALLOC_NET_PACKET();
	*msg << type << status;

	_service->SendDisconnect(session_id, msg);
	FREE_NET_SEND_PACKET(msg);
}
