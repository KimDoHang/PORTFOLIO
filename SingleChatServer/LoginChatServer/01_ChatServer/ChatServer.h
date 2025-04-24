#pragma once
#include "NetService.h"
#include "LockFreeQueue.h"
#include "ChatGroupJob.h"
#include "Player.h"
#include "ChatValues.h"
#include "NetSendPacket.h"
#include "ChatProtocol.h"
#include "ObjectPool.h"
#include "RedisUtils.h"
#include "RedisToken.h"


class MonitorClient;

class ChatServer : public NetService
{
	friend class ChatGroup;
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
	bool EnqueueChatGroupGob(ChatGroupJob chat_job);

	MonitorClient* GetMonitorClient() { return _monitor_client; }
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

	//Monitoring essential
	int32 _update_tps;
	uint64 _update_avg;
	//PDH
	uint64 _tcp_retransmission_avg;
private:
	int32 _player_num;
	int32 _max_player_num;
private:
	MonitorClient* _monitor_client;
	ChatGroup* _chat_group;
};

//LockFreeObjectPool -> ObjectPool로 변경 가능... ChatServer에서는 

extern 	RedisUtils* g_redis;
extern ChatServer* g_chatserver;


