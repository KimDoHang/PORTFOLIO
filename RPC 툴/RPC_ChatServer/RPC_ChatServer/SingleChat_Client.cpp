#include "pch.h"
#include "SingleChat_Client.h"
#include "TextParser.h"
#include "LogUtils.h"
#include "SingleChat_Packet.h"


void SingleChat_Client::OnConnect()
{
}

void SingleChat_Client::OnRelease()
{
}

void SingleChat_Client::OnRecvMsg(NetSerializeBuffer* msg)
{
	WORD type = UINT16_MAX;

	try
	{
		*msg >> type;

		switch (type)
		{
		case en_PACKET_SC_CHATRES_LOGIN:
			UnMarshal_ChatRes_Login(msg);
			break;
		case en_PACKET_SC_CHATRES_SECTORMOVE:
			UnMarshal_ChatRes_SectorMove(msg);
			break;
		case en_PACKET_SC_CHATRES_MESSAGE:
			UnMarshal_ChatRes_Message(msg);
			break;
		default:
			break;
		}
	}
	catch (const NetMsgException& net_msg_exception)
	{

	}
}

void SingleChat_Client::OnError(const int8 log_level, int32 err_code, const WCHAR* cause)
{
}

void SingleChat_Client::OnExit()
{

}
void SingleChat_Client::Marshal_ChatReq_Login(WCHAR* id, WCHAR* nick_name, char* session_key)
{
	NetSerializeBuffer* msg = ALLOC_NET_PACKET();

	(*msg) << en_PACKET_CS_CHATREQ_LOGIN;
	msg->PutData(reinterpret_cast<char*>(id), 20);
	msg->PutData(reinterpret_cast<char*>(nick_name), 20);
    msg->PutData(session_key, 64);

	SendPacket(msg);
	FREE_NET_SEND_PACKET(msg);
}
void SingleChat_Client::Marshal_ChatReq_SectorMove(int64 account_num, uint16 sector_x, uint16 sector_y)
{
	NetSerializeBuffer* msg = ALLOC_NET_PACKET();

	(*msg) << en_PACKET_CS_CHATREQ_SECTORMOVE;
    (*msg) << account_num;
    (*msg) << sector_x;
    (*msg) << sector_y;

	SendPacket(msg);
	FREE_NET_SEND_PACKET(msg);
}
void SingleChat_Client::Marshal_ChatReq_Message(int64 account_num, uint16 message_len, WCHAR* message)
{
	NetSerializeBuffer* msg = ALLOC_NET_PACKET();

	(*msg) << en_PACKET_CS_CHATREQ_MESSAGE;
    (*msg) << account_num;
    (*msg) << message_len;
	msg->PutData(reinterpret_cast<char*>(message), message_len / 2);

	SendPacket(msg);
	FREE_NET_SEND_PACKET(msg);
}
void SingleChat_Client::Marshal_ChatReq_HeartBeat()
{
	NetSerializeBuffer* msg = ALLOC_NET_PACKET();

	(*msg) << en_PACKET_CS_CHATREQ_HEARTBEAT;

	SendPacket(msg);
	FREE_NET_SEND_PACKET(msg);
}
void SingleChat_Client::UnMarshal_ChatRes_Login(NetSerializeBuffer* msg)
{
	uint8 status;
	int64 account_num;
	
	(*msg) >> status;
	(*msg) >> account_num;
	
	Handle_ChatRes_Login(status, account_num);
}
void SingleChat_Client::UnMarshal_ChatRes_SectorMove(NetSerializeBuffer* msg)
{
	int64 account_num;
	uint16 sector_x;
	uint16 sector_y;
	
	(*msg) >> account_num;
	(*msg) >> sector_x;
	(*msg) >> sector_y;
	
	Handle_ChatRes_SectorMove(account_num, sector_x, sector_y);
}
void SingleChat_Client::UnMarshal_ChatRes_Message(NetSerializeBuffer* msg)
{
	int64 account_num;	
	WCHAR id[20];	
	WCHAR nick_name[20];
	uint16 message_len;
	
	(*msg) >> account_num;
	msg->GetData(reinterpret_cast<char*>(id), 20);
	msg->GetData(reinterpret_cast<char*>(nick_name), 20);
	(*msg) >> message_len;
	WCHAR* message = new WCHAR[message_len / 2];
	msg->GetData(reinterpret_cast<char*>(message), message_len / 2);
	
	Handle_ChatRes_Message(account_num, id, nick_name, message_len, message);	
	delete[] message;
}
void SingleChat_Client::Handle_ChatRes_Login(uint8 status, int64 account_num)
{
}
void SingleChat_Client::Handle_ChatRes_SectorMove(int64 account_num, uint16 sector_x, uint16 sector_y)
{
}
void SingleChat_Client::Handle_ChatRes_Message(int64 account_num, WCHAR* id, WCHAR* nick_name, uint16 message_len, WCHAR* message)
{
}