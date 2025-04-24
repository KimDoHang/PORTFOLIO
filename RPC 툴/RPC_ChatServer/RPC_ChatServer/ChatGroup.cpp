#include "pch.h"
#include "NetService.h"
#include "ChatGroup.h"
#include "SingleChat_Packet.h"
#include "RedisToken.h"
#include "MonitorClient.h"

void ChatGroup::OnEnter(uint64 session_id, SessionInstance* instance)
{
}

void ChatGroup::OnLeave(uint64 session_id, SessionInstance* instance)
{
}

void ChatGroup::OnRelease(uint64 session_id, SessionInstance* instance)
{
	Player* player = reinterpret_cast<Player*>(instance);

	if (player == nullptr)
	{
		return;
	}

	SingleChat_Server* server = reinterpret_cast<SingleChat_Server*>(_service);

	if (player->_sector_x != UINT16_MAX)
	{
		list<Player*>* sector = &_sectors[player->_sector_y][player->_sector_x];

		for (auto iter = sector->begin(); iter != sector->end();)
		{
			if (*iter == player)
			{
				sector->erase(iter);
				break;
			}
			else
				iter++;
		}
	}

	_players.erase(session_id);
	_player_pool.Free(player);

	AtomicDecrement32(&server->_player_num);
}

void ChatGroup::OnFrame()
{
	long size = 0;

	ChatGroupJob job;
	SingleChat_Server* server = reinterpret_cast<SingleChat_Server*>(_service);
	AtomicIncrement32(&server->_update_tps);

	while (true)
	{
		size = _update_job_queue.Size();

		if (size == 0)
		{
			break;
		}

		_update_job_queue.Dequeue(job);

		switch (job._type)
		{
		case en_JOB_CREATE_SESSION:
			HandlePlayerCreate(job._session_id, job._login_success);
			break;
		case en_JOB_GUI_REQ_SECTOR_INFO:
			HandleSectorInfo();
			break;
		default:
			_service->OnError(dfLOG_LEVEL_SYSTEM, job._session_id, 0, L"ChatServer ChatGroup Job Type Error");
			Disconnect(job._session_id);
			break;
		}

	}
}

bool ChatGroup::OnRecvMsg(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg)
{
	WORD type = UINT16_MAX;

	try
	{
		*msg >> type;

		switch (type)
		{
		case en_PACKET_CS_CHATREQ_LOGIN:
			UnMarshal_ChatReq_Login(session_id, instance, msg);
			break;
		case en_PACKET_CS_CHATREQ_SECTORMOVE:
			UnMarshal_ChatReq_SectorMove(session_id, instance, msg);
			break;
		case en_PACKET_CS_CHATREQ_MESSAGE:
			UnMarshal_ChatReq_Message(session_id, instance, msg);
			break;
		case en_PACKET_CS_CHATREQ_HEARTBEAT:
			UnMarshal_ChatReq_HeartBeat(session_id, instance, msg);
			break;
		default:
			_service->OnError(dfLOG_LEVEL_SYSTEM, session_id, 0, L"ChatServer MSG Type Error");
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

void ChatGroup::OnCheckSessionkey(cpp_redis::reply&& reply, void* callback_data)
{
	ChatGroupJob job;
	TokenObject* token = reinterpret_cast<TokenObject*>(callback_data);

	bool flag = false;
	do
	{
		//Get ���� ���� Ȯ��
		if (reply.is_null())
		{
			g_chatserver->OnError(dfLOG_LEVEL_SYSTEM, token->session_id, 0, L"Token Not Exist");
			break;
		}

		char key[df_SESSION_KEY_BUFF_SIZE];
		sprintf_s(key, df_SESSION_KEY_BUFF_SIZE, "ChatServer:%I64d", token->session_id);

		if (strncmp(token->session_key, reply.as_string().c_str(), 64) != 0)
		{
			g_redis->DelSync(key);
			g_chatserver->OnError(dfLOG_LEVEL_SYSTEM, token->session_id, 0, L"Token Not Equal");
			break;
		}

		flag = true;
		g_redis->DelSync(key);

	} while (false);


	//����, ������ ���� ����� ����
	job.JobInit(en_JOB_CREATE_SESSION, token->session_id, flag);
	_token_pool.Free(token);

	g_chatserver->EnqueueChatGroupGob(job);
}


void ChatGroup::Marshal_ChatRes_Login(uint64 session_id, uint8 status, int64 account_num)
{
	NetSerializeBuffer* msg = ALLOC_NET_PACKET();

	(*msg) << en_PACKET_SC_CHATRES_LOGIN;
    (*msg) << status;
    (*msg) << account_num;

	_service->SendPacket(session_id, msg);
	FREE_NET_SEND_PACKET(msg);
}
void ChatGroup::Marshal_ChatRes_SectorMove(uint64 session_id, int64 account_num, uint16 sector_x, uint16 sector_y)
{
	NetSerializeBuffer* msg = ALLOC_NET_PACKET();

	(*msg) << en_PACKET_SC_CHATRES_SECTORMOVE;
    (*msg) << account_num;
    (*msg) << sector_x;
    (*msg) << sector_y;

	_service->SendPacket(session_id, msg);
	FREE_NET_SEND_PACKET(msg);
}
void ChatGroup::Marshal_ChatRes_Message(uint64 session_id, int64 account_num, WCHAR* id, WCHAR* nick_name, uint16 message_len, WCHAR* message)
{
	NetSerializeBuffer* msg = ALLOC_NET_PACKET();

	(*msg) << en_PACKET_SC_CHATRES_MESSAGE;
    (*msg) << account_num;
	msg->PutData(reinterpret_cast<char*>(id), 20 * 2);
	msg->PutData(reinterpret_cast<char*>(nick_name), 20 * 2);
    (*msg) << message_len;
	msg->PutData(reinterpret_cast<char*>(message), message_len / 2 * 2);

	_service->SendPacket(session_id, msg);
	FREE_NET_SEND_PACKET(msg);
}
void ChatGroup::UnMarshal_ChatReq_Login(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg)
{	
	int64 account_num;
	WCHAR id[20];	
	WCHAR nick_name[20];	
    char session_key[64];

	(*msg) >> account_num;
	msg->GetData(reinterpret_cast<char*>(id), 20 * 2);
	msg->GetData(reinterpret_cast<char*>(nick_name), 20 * 2);
    msg->GetData(session_key, 64);
	
    Handle_ChatReq_Login(session_id, instance, account_num, id, nick_name, session_key);
        
}
void ChatGroup::UnMarshal_ChatReq_SectorMove(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg)
{
	int64 account_num;
	uint16 sector_x;
	uint16 sector_y;

	
	(*msg) >> account_num;
	(*msg) >> sector_x;
	(*msg) >> sector_y;
	
    Handle_ChatReq_SectorMove(session_id, instance, account_num, sector_x, sector_y);
        
}
void ChatGroup::UnMarshal_ChatReq_Message(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg)
{
	int64 account_num;
	uint16 message_len;

	
	(*msg) >> account_num;
	(*msg) >> message_len;
	WCHAR* message = new WCHAR[message_len / 2 * 2];
	msg->GetData(reinterpret_cast<char*>(message), message_len / 2 * 2);
	
    Handle_ChatReq_Message(session_id, instance, account_num, message_len, message);
        	
	delete[] message;
}
void ChatGroup::UnMarshal_ChatReq_HeartBeat(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg)
{

	
	
    Handle_ChatReq_HeartBeat(session_id, instance);
}
void ChatGroup::Handle_ChatReq_Login(uint64 session_id, SessionInstance* instance, int64 account_num, WCHAR* id, WCHAR* nick_name, char* session_key)
{
	SingleChat_Server* server = reinterpret_cast<SingleChat_Server*>(_service);

	if (AtomicIncrement32(&server->_player_num) >= server->_max_player_num)
	{
		AtomicDecrement32(&server->_player_num);
		Disconnect(session_id);
		_service->OnError(dfLOG_LEVEL_SYSTEM, session_id, df_CHAT_MAX_PLAYER_ERROR, L"MAX Player");
		return;
	}

	AtomicIncrement32(&server->_req_login_tps);

	Player* player = _player_pool.Alloc();
	player->InitPlayer(session_id, account_num, id, nick_name);
	_players[session_id] = player;
	_service->RegisterSessionInstance(session_id, player);

	CheckSessionKey(account_num, session_id, session_key);
}
void ChatGroup::Handle_ChatReq_SectorMove(uint64 session_id, SessionInstance* instance, int64 account_num, uint16 sector_x, uint16 sector_y)
{
	Player* player = reinterpret_cast<Player*>(instance);

	do
	{
		if (player->_login_flag == false)
		{
			_service->OnError(dfLOG_LEVEL_SYSTEM, session_id, df_CHAT_NOTLOGIN_ERROR, L"Wrong Account");
			break;
		}

		if (player->_account_num != account_num)
		{
			_service->OnError(dfLOG_LEVEL_SYSTEM, session_id, df_CHAT_ACCOUNT_ERROR, L"Wrong Account");
			break;
		}

		//uint16�̹Ƿ� 0<�� �� �ʿ䰡 ����.
		if (sector_x >= MAX_SECTOR_SIZE_X || sector_y >= MAX_SECTOR_SIZE_Y)
		{
			_service->OnError(dfLOG_LEVEL_SYSTEM, session_id, df_CHAT_SECTORIDX_ERROR, L"Wrong Sector Pos");
			break;
		}

		if (player->_sector_x != UINT16_MAX)
		{
			list<Player*>* sector = &_sectors[player->_sector_y][player->_sector_x];

			for (auto iter = sector->begin(); iter != sector->end(); )
			{
				if (player == *iter)
				{
					sector->erase(iter);
					break;
				}
				else
					iter++;
			}
		}

		player->_sector_x = sector_x;
		player->_sector_y = sector_y;

		_sectors[sector_y][sector_x].push_back(player);

		Marshal_ChatRes_SectorMove(session_id, account_num, sector_x, sector_y);
		return;
	} while (false);

	Disconnect(session_id);
}
void ChatGroup::Handle_ChatReq_Message(uint64 session_id, SessionInstance* instance, int64 account_num, uint16 message_len, WCHAR* message)
{
	Player* player = reinterpret_cast<Player*>(instance);


	do
	{

		if (player->_login_flag == false)
		{
			_service->OnError(dfLOG_LEVEL_SYSTEM, session_id, df_CHAT_NOTLOGIN_ERROR, L"Not Login");
			break;
		}

		if (player->_account_num != account_num)
		{
			_service->OnError(dfLOG_LEVEL_SYSTEM, session_id, df_CHAT_ACCOUNT_ERROR, L"Wrong Account");
			break;
		}

		if (player->_sector_x == UINT16_MAX)
		{
			_service->OnError(dfLOG_LEVEL_SYSTEM, session_id, df_CHAT_ACCOUNT_ERROR, L"Wrong Account");
			break;
		}


		MarshalResMessage(session_id, en_PACKET_SC_CHATRES_MESSAGE, account_num, player->_id, player->_nick_name, message_len, message, player->_sector_x, player->_sector_y);
		return;
	} while (false);


	Disconnect(session_id);
	return;
}
void ChatGroup::Handle_ChatReq_HeartBeat(uint64 session_id, SessionInstance* instance)
{
}

void ChatGroup::MarshalResMessage(uint64 session_id, uint16 type, int64 account_num, WCHAR* id, WCHAR* nick_name, uint16 message_len, WCHAR* message, uint16 sector_x, uint16 sector_y)
{
	NetSerializeBuffer* send_msg = ALLOC_NET_PACKET();

	*(send_msg) << type << account_num;
	send_msg->PutData(reinterpret_cast<char*>(id), MAX_ID_LENGTH);
	send_msg->PutData(reinterpret_cast<char*>(nick_name), MAX_NICKNAME_LENGTH);
	*(send_msg) << message_len;
	send_msg->PutData(reinterpret_cast<char*>(message), message_len);

	SendPacketSector(sector_x, sector_y, send_msg);
	FREE_NET_SEND_PACKET(send_msg);
}

void ChatGroup::HandlePlayerCreate(uint64 session_id, bool success)
{
	Player* player = GetPlayer(session_id);

	if (player == nullptr)
	{
		return;
	}

	SingleChat_Server* server = reinterpret_cast<SingleChat_Server*>(_service);

	//msg�� ������ ó���ص� ������ �������� ���� �̷��� ��� ä��

	//���нÿ��� ������ �ϹǷ� ���� ������.
	if (success == false)
	{
		MarshalResLoginDisconnect(session_id, en_PACKET_SC_CHATRES_LOGIN, success, player->_account_num);
		return;
	}

	Marshal_ChatRes_Login(session_id, success, player->_account_num);

	player->_login_flag = true;
}

#pragma warning (disable : 4244)

void ChatGroup::HandleSectorInfo()
{
	SingleChat_Server* server = reinterpret_cast<SingleChat_Server*>(_service);
	MonitorClient* client = server->GetMonitorClient();
	if (client->GetConnectFlag())
	{
		time_t tt;
		int32 cur_time = time(&tt);

		uint16 sector_infos[MAX_SECTOR_SIZE_Y][MAX_SECTOR_SIZE_X];

		for (int y = 0; y < MAX_SECTOR_SIZE_Y; y++)
		{
			for (int x = 0; x < MAX_SECTOR_SIZE_X; x++)
			{
				sector_infos[y][x] = (uint16)_sectors[y][x].size();
			}
		}

		client->MarshalReqSectorInfo(en_PACKET_SS_MONITOR_SECTOR_INFO, ChatServerNo, cur_time, sector_infos);
	}
}
#pragma warning (default : 4244)

void ChatGroup::MarshalResLoginDisconnect(uint64 session_id, uint16 type, uint8 status, int64 account_num)
{
	NetSerializeBuffer* send_msg = ALLOC_NET_PACKET();
	*send_msg << type << status << account_num;
	_service->SendDisconnectGroup(session_id, send_msg);
	FREE_NET_SEND_PACKET(send_msg);
}

void ChatGroup::CheckSessionKey(int64 account_num, uint64 session_id, char* session_key)
{
	//session key�� NULL�� �ƴ� ���� ũ��� �������Ƿ� memcpy�� ����ؾ� �Ѵ�.
	TokenObject* token = _token_pool.Alloc(session_id, session_key);

	char key[df_SESSION_KEY_BUFF_SIZE];
	sprintf_s(key, df_SESSION_KEY_BUFF_SIZE, "ChatServer:%I64d", account_num);


	//Redis Thread�� redis�� key, token, callback �Լ����� (Redis Job ���·� Redis JobQueue�� �����ش�.
	if (g_redis->GetAsync(key, reinterpret_cast<void*>(token), ChatGroup::OnCheckSessionkey) == false)
	{
		_token_pool.Free(token);
		_service->OnError(dfLOG_LEVEL_SYSTEM, df_ERROR_REDIS_JOB_QUEUE_FULL, L"Redis Job Queue Full");
		__debugbreak();
	}
}
