#include "pch.h"
#include "ChatSolo.h"

void ChatSolo::OnEnter(uint64 session_id, SessionInstance* instance)
{
	ChatServer* server = reinterpret_cast<ChatServer*>(_service);

	if (AtomicIncrement32(&server->_player_num) >= server->_max_player_num)
	{
		Disconnect(session_id);
		_service->OnError(dfLOG_LEVEL_SYSTEM, session_id, df_CHAT_MAX_PLAYER_ERROR, L"MAX Player");
		return;
	}

	MarshalResLogin(session_id, en_PACKET_CS_CHAT_RES_LOGIN, true, reinterpret_cast<Player*>(instance)->_account_num);
	reinterpret_cast<Player*>(instance)->_login_flag = true;
}

void ChatSolo::OnLeave(uint64 session_id, SessionInstance* instance)
{
	ChatServer* server = reinterpret_cast<ChatServer*>(_service);
	AtomicDecrement32(&server->_player_num);
}

void ChatSolo::OnRelease(uint64 session_id, SessionInstance* instance)
{
	ChatServer* server = reinterpret_cast<ChatServer*>(_service);
	Player* player = server->GetPlayer(session_id);
	AtomicDecrement32(&server->_player_num);

	//ReqLogin이 처리되기 전에 Release가 호출된 경우 없을 수 있다. (TimeOut을 내부에서 처리하기 때문에 발생)
	if (player == nullptr)
		return;

	//삭제시 섹터를 먼저 지워야지 쓸대없는 msg가 가지 않는다. 이것이 더 급하다..
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
		//재접속 방지를 위해 추가적인 flag를 둘 수 있다.
		SrwLockExclusiveGuard exclusive_guard(&ChatServer::_player_lock);
		ChatServer::_players.erase(session_id);
	}

	ChatServer::_player_pool.Free(player);
	return;
}

bool ChatSolo::OnRecvMsg(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg)
{
	WORD type = UINT16_MAX;

	try
	{
		*msg >> type;

		switch (type)
		{
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
			_service->OnError(dfLOG_LEVEL_SYSTEM, session_id, 0, L"ChatServer ChatSolo MSG Type Error");
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

void ChatSolo::UnmarshalReqSectorMove(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg)
{
	int64 account_num;
	uint16 sector_x;
	uint16 sector_y;

	*(msg) >> account_num;
	*(msg) >> sector_x;
	*(msg) >> sector_y;

	HandleReqSectorMove(session_id, instance, account_num, sector_x, sector_y);
}

void ChatSolo::UnmarshalReqMessage(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg)
{
	int64 account_num;
	WORD msg_len;

	*(msg) >> account_num;
	*(msg) >> msg_len;

	WCHAR* msg_start = reinterpret_cast<WCHAR*>(msg->GetReadPtr());

	HandleReqMessage(session_id, instance, account_num, msg_len, msg_start);
}

void ChatSolo::MarshalResLogin(uint64 session_id, uint16 type, uint8 status, int64 account_num)
{
	NetSerializeBuffer* send_msg = ALLOC_NET_PACKET();
	*send_msg << type << status << account_num;
	_service->SendPacket(session_id, send_msg);
	FREE_NET_SEND_PACKET(send_msg);
}

void ChatSolo::MarshalResSectorMove(uint64 session_id, uint16 type, int64 account_num, uint16 sector_x, uint16 sector_y)
{
	NetSerializeBuffer* send_msg = ALLOC_NET_PACKET();
	*send_msg << type << account_num << sector_x << sector_y;
	_service->SendPacket(session_id, send_msg);
	FREE_NET_SEND_PACKET(send_msg);
}

void ChatSolo::MarshalResMessage(uint64 session_id, uint16 type, int64 account_num, WCHAR* id, WCHAR* nick_name, uint16 message_len, WCHAR* message, uint16 sector_x, uint16 sector_y)
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


void ChatSolo::HandlePlayerCreate(uint64 session_id, bool success)
{
}

void ChatSolo::HandleReqSectorMove(uint64 session_id, SessionInstance* instance, int64 account_num, uint16 sector_x, uint16 sector_y)
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

		if (player->_sector_x == UINT16_MAX)
		{
			player->_sector_x = sector_x;
			player->_sector_y = sector_y;

			{
				SrwLockExclusiveGuard exclusive_guard(&ChatServer::_sector_lock[sector_y][sector_x]);
				ChatServer::_sectors[sector_y][sector_x].push_back(player);
			}

			MarshalResSectorMove(session_id, en_PACKET_CS_CHAT_RES_SECTOR_MOVE, account_num, sector_x, sector_y);
			return;
		}

		Pos pos[2];

		if (player->_sector_x + player->_sector_y * MAX_SECTOR_SIZE_X < sector_x + sector_y * MAX_SECTOR_SIZE_X)
		{
			pos[0].x = player->_sector_x;
			pos[0].y = player->_sector_y;
			pos[1].x = sector_x;
			pos[1].y = sector_y;
		}
		else if (player->_sector_x + player->_sector_y * MAX_SECTOR_SIZE_X > sector_x + sector_y * MAX_SECTOR_SIZE_X)
		{
			pos[0].x = sector_x;
			pos[0].y = sector_y;
			pos[1].x = player->_sector_x;
			pos[1].y = player->_sector_y;
		}
		else
		{
			MarshalResSectorMove(session_id, en_PACKET_CS_CHAT_RES_SECTOR_MOVE, account_num, sector_x, sector_y);
			return;
		}

		{
			SrwLockExclusiveGuard exclusive_guard1(&ChatServer::_sector_lock[pos[0].y][pos[0].x]);
			SrwLockExclusiveGuard exclusive_guard2(&ChatServer::_sector_lock[pos[1].y][pos[1].x]);

			list<Player*>* sector = &ChatServer::_sectors[player->_sector_y][player->_sector_x];

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

			ChatServer::_sectors[sector_y][sector_x].push_back(player);

			player->_sector_x = sector_x;
			player->_sector_y = sector_y;
		}


		MarshalResSectorMove(session_id, en_PACKET_CS_CHAT_RES_SECTOR_MOVE, account_num, sector_x, sector_y);
		return;

	} while (false);

	Disconnect(session_id);
	return;
}

void ChatSolo::HandleReqMessage(uint64 session_id, SessionInstance* instance, int64 account_num, uint16 message_len, WCHAR* message)
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

void ChatSolo::HandleReqHeartBeat(uint64 session_id, SessionInstance* instance)
{
}



