#include "pch.h"
#include "ChatGroup.h"

void ChatGroup::OnCheckSessionkey(RedisRet reply, void* callback_data)
{
	ChatGroupJob job;
	TokenObject* token = reinterpret_cast<TokenObject*>(callback_data);

	bool flag = false;
	do
	{
		//Get 실패 여부 확인
		if (reply._status == 0)
		{
			g_chatserver->OnError(dfLOG_LEVEL_SYSTEM, token->session_id, 0, L"Token Not Exist");
			break;
		}

		char key[df_SESSION_KEY_BUFF_SIZE];
		sprintf_s(key, df_SESSION_KEY_BUFF_SIZE, "ChatServer:%I64d", token->session_id);

		if (strncmp(token->session_key, reply._val, 64) != 0)
		{
			g_redis->DelSync(RedisServerType::RedisChatServer, token->key);
			g_chatserver->OnError(dfLOG_LEVEL_SYSTEM, token->session_id, 0, L"Token Not Equal");
			break;
		}

		flag = true;
		g_redis->DelSync(RedisServerType::RedisChatServer, token->key);

	} while (false);


	//실패, 성공에 따른 결과값 전송
	job.JobInit(en_PACKET_CREATE_SESSION, token->session_id, flag);
	_token_pool.Free(token);

	g_chatserver->EnqueueChatGroupGob(job);
}

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

	ChatServer* server = reinterpret_cast<ChatServer*>(_service);

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

bool ChatGroup::OnRecvMsg(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg)
{
	WORD type = UINT16_MAX;

	try
	{
		*msg >> type;

		switch (type)
		{
		case en_PACKET_CS_CHAT_REQ_LOGIN:
			UnmarshalReqLogin(session_id, instance, msg);
			break;
		case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
			UnmarshalReqSectorMove(session_id, instance, msg);
			break;
		case en_PACKET_CS_CHAT_REQ_MESSAGE:
			UnmarshalReqMessage(session_id, instance, msg);
			break;
		case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
			HandleReqHeartBeat(session_id, instance);
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

void ChatGroup::OnFrame()
{
	long size = 0;

	ChatGroupJob job;
	ChatServer* server = reinterpret_cast<ChatServer*>(_service);
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
		case en_PACKET_CREATE_SESSION:
			HandlePlayerCreate(job._session_id, job._login_success);
			break;
			_service->OnError(dfLOG_LEVEL_SYSTEM, job._session_id, 0, L"ChatServer ChatGroup Job Type Error");
			Disconnect(job._session_id);
			break;
		}

	}
}

void ChatGroup::UnmarshalReqLogin(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg)
{
	int64 account_num;
	WCHAR id[20];
	WCHAR nick_name[20];
	char session_key[64];

	*(msg) >> account_num;
	msg->GetData(reinterpret_cast<char*>(&id), MAX_ID_LENGTH);
	msg->GetData(reinterpret_cast<char*>(&nick_name), MAX_NICKNAME_LENGTH);
	msg->GetData(reinterpret_cast<char*>(&session_key), MAX_SESSIONKEY_LENGTH);

	HandleReqLogin(session_id, instance, account_num, id, nick_name, session_key);

}

void ChatGroup::UnmarshalReqSectorMove(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg)
{
	int64 account_num;
	uint16 sector_x;
	uint16 sector_y;

	*(msg) >> account_num;
	*(msg) >> sector_x;
	*(msg) >> sector_y;

	HandleReqSectorMove(session_id, instance, account_num, sector_x, sector_y);
}

void ChatGroup::UnmarshalReqMessage(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg)
{
	int64 account_num;
	WORD msg_len;

	*(msg) >> account_num;
	*(msg) >> msg_len;

	WCHAR* msg_start = reinterpret_cast<WCHAR*>(msg->GetReadPtr());

	HandleReqMessage(session_id, instance, account_num, msg_len, msg_start);
}

void ChatGroup::MarshalResLogin(uint64 session_id, uint16 type, uint8 status, int64 account_num)
{
	NetSerializeBuffer* send_msg = ALLOC_NET_PACKET();
	*send_msg << type << status << account_num;
	_service->SendPacket(session_id, send_msg);
	FREE_NET_SEND_PACKET(send_msg);
}

void ChatGroup::MarshalResLoginDisconnect(uint64 session_id, uint16 type, uint8 status, int64 account_num)
{
	NetSerializeBuffer* send_msg = ALLOC_NET_PACKET();
	*send_msg << type << status << account_num;
	_service->SendDisconnectGroup(session_id, send_msg);
	FREE_NET_SEND_PACKET(send_msg);
}

void ChatGroup::MarshalResSectorMove(uint64 session_id, uint16 type, int64 account_num, uint16 sector_x, uint16 sector_y)
{
	NetSerializeBuffer* send_msg = ALLOC_NET_PACKET();
	*send_msg << type << account_num << sector_x << sector_y;
	_service->SendPacket(session_id, send_msg);
	FREE_NET_SEND_PACKET(send_msg);
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

void ChatGroup::HandleReqLogin(uint64 session_id, SessionInstance* instance, int64 account_num, WCHAR* id, WCHAR* nick_name, char* session_key)
{
	ChatServer* server = reinterpret_cast<ChatServer*>(_service);

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

	CheckSessionKey(id, session_id, session_key);
}

void ChatGroup::HandleReqSectorMove(uint64 session_id, SessionInstance* instance, int64 account_num, uint16 sector_x, uint16 sector_y)
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

		//uint16이므로 0<는 볼 필요가 없다.
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

		MarshalResSectorMove(session_id, en_PACKET_CS_CHAT_RES_SECTOR_MOVE, account_num, sector_x, sector_y);
		return;
	} while (false);

	Disconnect(session_id);
	return;
}

void ChatGroup::HandleReqMessage(uint64 session_id, SessionInstance* instance, int64 account_num, uint16 message_len, WCHAR* message)
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


		MarshalResMessage(session_id, en_PACKET_CS_CHAT_RES_MESSAGE, account_num, player->_id, player->_nick_name, message_len, message, player->_sector_x, player->_sector_y);
		return;
	} while (false);

	
	Disconnect(session_id);
	return;
}

void ChatGroup::HandleReqHeartBeat(uint64 session_id, SessionInstance* instance)
{



}

void ChatGroup::HandlePlayerCreate(uint64 session_id, bool success)
{
	Player* player = GetPlayer(session_id);

	if (player == nullptr)
	{
		return;
	}

	ChatServer* server = reinterpret_cast<ChatServer*>(_service);

	//msg를 보내고 처리해도 되지만 안전성을 위해 이러한 방식 채택

	//실패시에도 보내야 하므로 먼저 보낸다.
	if (success == false)
	{
		MarshalResLoginDisconnect(session_id, en_PACKET_CS_CHAT_RES_LOGIN, success, player->_account_num);
		return;
	}

	MarshalResLogin(session_id, en_PACKET_CS_CHAT_RES_LOGIN, success, player->_account_num);

	player->_login_flag = true;
}

void ChatGroup::CheckSessionKey(WCHAR* id, uint64 session_id, char* session_key)
{
	//session key는 NULL이 아닌 버퍼 크기로 정해지므로 memcpy를 사용해야 한다.

	if (wcsncmp(id, L"ID_", 3) != 0)
	{
		_service->OnError(dfLOG_LEVEL_SYSTEM, 0, L"ID data Error");
		Disconnect(session_id);
		return;
	}

	int64 key = _wtoi(id + 3);

	//if (key > MAX_ID_BOUND)
	//{
	//	_service->OnError(dfLOG_LEVEL_SYSTEM, 0, L"ID data Error");
	//	Disconnect(session_id);
	//	return;
	//}


	TokenObject* token = _token_pool.Alloc(key, session_id, session_key);


	//Redis Thread로 redis의 key, token, callback 함수전달 (Redis Job 형태로 Redis JobQueue로 던져준다.
	if (g_redis->GetAsync(RedisServerType::RedisChatServer, key, reinterpret_cast<void*>(token), ChatGroup::OnCheckSessionkey) == false)
	{
		_token_pool.Free(token);
		_service->OnError(dfLOG_LEVEL_SYSTEM, df_ERROR_REDIS_JOB_QUEUE_FULL, L"Redis Job Queue Full");
		__debugbreak();
	}
}


