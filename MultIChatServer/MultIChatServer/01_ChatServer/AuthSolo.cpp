#include "pch.h"
#include "AuthSolo.h"
#include "RedisUtils.h"
#include "ChatSolo.h"

void AuthSolo::OnEnter(uint64 session_id, SessionInstance* instance)
{
	ChatServer* server = reinterpret_cast<ChatServer*>(_service);
	AtomicIncrement32(&server->_auth_num);
}

void AuthSolo::OnLeave(uint64 session_id, SessionInstance* instance)
{
	ChatServer* server = reinterpret_cast<ChatServer*>(_service);
	AtomicDecrement32(&server->_auth_num);
}

void AuthSolo::OnRelease(uint64 session_id, SessionInstance* instance)
{

	ChatServer* server = reinterpret_cast<ChatServer*>(_service);

	AtomicDecrement32(&server->_auth_num);

	Player* player = server->GetPlayer(session_id);

	//ReqLogin�� ó���Ǳ� ���� Release�� ȣ��� ��� ���� �� �ִ�. (TimeOut�� ���ο��� ó���ϱ� ������ �߻�)
	if (player == nullptr)
	{
		return;
	}

	//������ ���͸� ���� �������� ������� msg�� ���� �ʴ´�. �̰��� �� ���ϴ�..
	if (player->_sector_x != UINT16_MAX)
	{
		list<Player*>* sector = &ChatServer::_sectors[player->_sector_y][player->_sector_x];

		SrwLockExclusiveGuard exclusive_guard(&ChatServer::_sector_lock[player->_sector_y][player->_sector_x]);
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

	{
		//������ ������ ���� �߰����� flag�� �� �� �ִ�.
		SrwLockExclusiveGuard exclusive_guard(&ChatServer::_player_lock);
		ChatServer::_players.erase(session_id);
	}

	ChatServer::_player_pool.Free(player);

}

bool AuthSolo::OnRecvMsg(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg)
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

void AuthSolo::UnmarshalReqLogin(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg)
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



void AuthSolo::HandleReqLogin(uint64 session_id, SessionInstance* instance, int64 account_num, WCHAR* id, WCHAR* nick_name, char* session_key)
{
	ChatServer* server = reinterpret_cast<ChatServer*>(_service);


	AtomicIncrement32(&server->_req_login_tps);

	char key[df_SESSION_KEY_BUFF_SIZE];
	sprintf_s(key, df_SESSION_KEY_BUFF_SIZE, "ChatServer:%I64d", account_num);
	auto reply =  g_redis->GetSync(key);

	do
	{
		if (reply.is_null())
		{
			_service->OnError(dfLOG_LEVEL_SYSTEM, session_id, 0, L"Token Not Exist");
			return;
		}

		if (strncmp(session_key, reply.as_string().c_str(), 64) != 0)
		{
			g_redis->DelSync(key);
			g_chatserver->OnError(dfLOG_LEVEL_SYSTEM, session_id, 0, L"Token Not Equal");
			break;
		}
		g_redis->DelSync(key);

		Player* player = ChatServer::_player_pool.Alloc();
		player->InitPlayer(session_id, account_num, id, nick_name);

		{
			SrwLockExclusiveGuard exclusive_guard(&ChatServer::_player_lock);
			ChatServer::_players[session_id] = player;
		}

		_service->RegisterSessionInstance(session_id, player);
		
		server->_chat_solo->RegisterEnter(session_id, LogicType::SoloType);
		return;

	} while (false);

	MarshalResLoginDisconnect(session_id, en_PACKET_CS_CHAT_RES_LOGIN, false, account_num);
}


void AuthSolo::MarshalResLoginDisconnect(uint64 session_id, uint16 type, uint8 status, int64 account_num)
{
	NetSerializeBuffer* send_msg = ALLOC_NET_PACKET();
	*send_msg << type << status << account_num;
	_service->SendDisconnectGroup(session_id, send_msg);
	FREE_NET_SEND_PACKET(send_msg);
}
