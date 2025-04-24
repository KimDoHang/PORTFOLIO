#pragma once
#include "SoloInstance.h"
#include "NetSerializeBuffer.h"
#include "LoginServer.h"
#include "NetSendPacket.h"
#include "LoginServer.h"
#include "LoginValues.h"
class AuthServer : public SoloInstance
{
public:

	AuthServer(NetService* service, uint16 logic_id, WCHAR* game_server_ip, USHORT game_server_port, WCHAR* chat_server_ip, USHORT chat_port) : SoloInstance(service, logic_id), _game_server_port(game_server_port), _chat_server_port(chat_port)
	{
		_db_manager = new SqlUtils;
		_redis_manger = new RedisUtils;
		wcscpy_s(_game_server_ip, AUTH_IP_BUFF_SIZE, game_server_ip);
		wcscpy_s(_chat_server_ip, AUTH_IP_BUFF_SIZE, chat_server_ip);

	}

	~AuthServer()
	{
		delete _db_manager;
		delete _redis_manger;
	}
	

	virtual void OnEnter(uint64 session_id, SessionInstance* player) override;
	virtual void OnLeave(uint64 session_id, SessionInstance* player) override;
	virtual void OnRelease(uint64 session_id, SessionInstance* player) override;
	virtual bool OnRecvMsg(uint64 session_id, SessionInstance* player, NetSerializeBuffer* msg) override;

public:
	void UnmarshalReqLogin(uint64 session_id, NetSerializeBuffer* msg)
	{
		int64 account_num = 0;
		char session_key[df_SESSION_KEY_SIZE];
		*msg >> account_num;
		msg->GetData(session_key, df_SESSION_KEY_SIZE);

		HandleReqLogin(session_id, account_num, session_key);
	}

	__forceinline void MarshalResLogin(uint64 session_id, uint16 type, uint64 account_num, uint8 status, WCHAR* id, WCHAR* nick_name, WCHAR* game_ip, USHORT game_port, WCHAR* chat_ip, USHORT chat_port);
	__forceinline void MarshalResLoginDisconnect(uint64 session_id, uint16 type, uint64 account_num, uint8 status, WCHAR* id, WCHAR* nick_name, WCHAR* game_ip, USHORT game_port, WCHAR* chat_ip, USHORT chat_port);

	void HandleReqLogin(uint64 session_id, int64 account_num, char* session_key);

	WCHAR* GetGameIP() { return _game_server_ip; }
	WCHAR* GetChatIP() { return _chat_server_ip; }
	USHORT* GetSGamePort() { return &_game_server_port; }
	USHORT* GetChatPort() { return &_chat_server_port; }

private:
	BYTE CheckPlatform(uint64 account_num, char* session_key, WCHAR* id, WCHAR* nick_name);
	int32 TokenWrite(uint64 account_num, char* session_key);


private:
	WCHAR _game_server_ip[AUTH_IP_BUFF_SIZE];
	USHORT _game_server_port;
	WCHAR _chat_server_ip[AUTH_IP_BUFF_SIZE];
	USHORT _chat_server_port;
	SqlUtils* _db_manager;
	RedisUtils* _redis_manger;
};


