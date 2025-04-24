#pragma once
#include "NetSerializeBuffer.h"
#include "ChatServer.h"
#include "Player.h"
#include "NetSendPacket.h"
#include "SoloInstance.h"

class ChatSolo : public SoloInstance
{
public:
	ChatSolo(NetService* service, uint16 logic_id) : SoloInstance(service, logic_id)
	{

	}

	virtual void OnEnter(uint64 session_id, SessionInstance* instance);
	virtual void OnLeave(uint64 session_id, SessionInstance* instance);
	virtual void OnRelease(uint64 session_id, SessionInstance* instance);
	virtual bool OnRecvMsg(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg);


public:
	void UnmarshalReqSectorMove(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg);
	void UnmarshalReqMessage(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg);

	void MarshalResLogin(uint64 session_id, uint16 type, uint8 status, int64 account_num);
	void MarshalResSectorMove(uint64 session_id, uint16 type, int64 account_num, uint16 sector_x, uint16 sector_y);
	void MarshalResMessage(uint64 session_id, uint16 type, int64 account_num, WCHAR* id, WCHAR* nick_name, uint16 message_len, WCHAR* message, uint16 sector_x, uint16 sector_y);

	void HandlePlayerCreate(uint64 session_id, bool success);
	void HandleReqSectorMove(uint64 session_id, SessionInstance* instance, int64 account_num, uint16 sector_x, uint16 sector_y);
	void HandleReqMessage(uint64 session_id, SessionInstance* instance, int64 account_num, uint16 message_len, WCHAR* message);
	void HandleReqHeartBeat(uint64 session_id, SessionInstance* instance);
private:

	__forceinline void SendPacketSector(int32 sector_x, int32 sector_y, NetSerializeBuffer* msg)
	{

		int32 nx, ny;
		Pos pos[9];
		int size = 0;

		for (int32 dir = 0; dir <= 8; dir++)
		{
			nx = sector_x + dx[dir];
			ny = sector_y + dy[dir];

			if (nx < 0 || ny < 0 || nx >= MAX_SECTOR_SIZE_X || ny >= MAX_SECTOR_SIZE_Y)
				continue;

			pos[size++] = { nx, ny };
			AcquireSRWLockShared(&ChatServer::_sector_lock[ny][nx]);
		}
		//idx √ ±‚»≠
		for (int32 cnt = 0; cnt < size; cnt++)
		{
			nx = pos[cnt].x;
			ny = pos[cnt].y;

			for (auto iter = ChatServer::_sectors[ny][nx].begin(); iter != ChatServer::_sectors[ny][nx].end(); iter++)
			{
				//AtomicIncrement32(&ChatServer::_res_msg);
				_service->SendPacket((*iter)->GetSessionID(), msg);
			}
		}

		for (int32 cnt = 0; cnt < size; cnt++)
		{
			nx = pos[cnt].x;
			ny = pos[cnt].y;
			ReleaseSRWLockShared(&ChatServer::_sector_lock[ny][nx]);
		}
	}

};

