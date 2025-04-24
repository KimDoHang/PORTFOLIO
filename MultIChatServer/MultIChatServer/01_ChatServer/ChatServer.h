#pragma once
#include "NetService.h"
#include "LockFreeQueue.h"
#include "Player.h"
#include "list"
#include "ChatValues.h"
#include "NetSendPacket.h"
#include "ChatProtocol.h"
#include "RedisUtils.h"
#include "Lock.h"


class MonitorClient;

class AuthSolo;
class ChatSolo;

class ChatServer : public NetService
{
	friend class ChatSolo;
	friend class AuthSolo;

public:
	ChatServer();
	~ChatServer();
	void ChatInit(const WCHAR* text_file);
public:
	virtual bool OnAccept(uint64 session_id);
	virtual bool OnConnectionRequest(WCHAR* IP, SHORT port);
	virtual void OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause) override;
	virtual void OnError(const int8 log_level, int32 err_code, const WCHAR* cause) override;
	virtual bool OnTimeOut(uint64 session_id);
	virtual void OnMonitor() override;
	virtual void OnExit() override;

public:
	void ChatInit(int32 max_player_num);
	void ChatStart();

	__forceinline Player* GetPlayer(uint64 session_id)
	{
		SrwLockSharedGuard shared_guard(&ChatServer::_player_lock);

		auto player = ChatServer::_players.find(session_id);
		if (player == ChatServer::_players.end())
			return nullptr;

		return player->second;
	}
private:
	int32 _req_msg_tps;
	int32 _res_msg_tps;
	uint64 _req_msg_avg;
	uint64 _res_msg_avg;
	int32 _req_move_tps;
	int32 _res_move_tps;
	uint64 _req_move_avg;
	uint64 _res_move_avg;
	int32 _req_login_tps;
	uint64 _req_login_avg;
	//PDH
	uint64 _tcp_retransmission_avg;
private:
	int32 _auth_num;
	int32 _chat_num;
	int32 _player_num;
	int32 _max_player_num;
private:
	MonitorClient* _monitor_client;
	ChatSolo* _chat_solo;
	AuthSolo* _auth_solo;
public:
	static unordered_map<uint64, Player*> _players;
	static LockFreeObjectPoolTLS<Player, false> _player_pool;
	static list<Player*> _sectors[MAX_SECTOR_SIZE_Y][MAX_SECTOR_SIZE_X];
	static SRWLOCK _sector_lock[MAX_SECTOR_SIZE_Y][MAX_SECTOR_SIZE_X];
	static SRWLOCK _player_lock;
};

//LockFreeObjectPool -> ObjectPool로 변경 가능... ChatServer에서는 

extern 	RedisUtils* g_redis;
extern ChatServer* g_chatserver;


