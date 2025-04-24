#pragma once
#include "NetSerializeBuffer.h"
#include "ChatServer.h"
#include "Player.h"
#include "NetSendPacket.h"
#include "SoloInstance.h"

class AuthSolo : public SoloInstance
{
public:
	AuthSolo(NetService* service, uint16 logic_id) : SoloInstance(service, logic_id)
	{

	}

	virtual void OnEnter(uint64 session_id, SessionInstance* instance);
	virtual void OnLeave(uint64 session_id, SessionInstance* instance);
	virtual void OnRelease(uint64 session_id, SessionInstance* instance);
	virtual bool OnRecvMsg(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg);

public:
	void UnmarshalReqLogin(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg);

	void MarshalResLoginDisconnect(uint64 session_id, uint16 type, uint8 status, int64 account_num);

	void HandleReqLogin(uint64 session_id, SessionInstance* instance, int64 account_num, WCHAR* id, WCHAR* nick_name, char* session_key);
};

