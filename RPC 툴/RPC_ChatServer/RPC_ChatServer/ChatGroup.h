#pragma once
#include "GroupInstance.h"
#include "NetSendPacket.h"
#include "NetSerializeBuffer.h"
#include "Player.h"
#include "ObjectPool.h"
#include "ChatGroupJob.h"
#include "ChatValues.h"
#include "RedisUtils.h"
#include "SingleChat_Server.h"

class ChatGroup : public GroupInstance
{
public:
    static void OnCheckSessionkey(cpp_redis::reply&& reply, void* callback_data);
public:
    ChatGroup(NetService * service, uint16 logic_id, uint64 frame_tick) : GroupInstance(service, logic_id, frame_tick)
    {

    }

    virtual void OnEnter(uint64 session_id, SessionInstance * instance) override;
    virtual void OnLeave(uint64 session_id, SessionInstance * instance) override;
    virtual void OnRelease(uint64 session_id, SessionInstance * instance) override;
    virtual bool OnRecvMsg(uint64 session_id, SessionInstance * instance, NetSerializeBuffer * msg) override;
    virtual void OnFrame();

public:
	//RPC肺 积己等 何盒
    void Marshal_ChatRes_Login(uint64 session_id,uint8 status, int64 account_num);
    void Marshal_ChatRes_SectorMove(uint64 session_id,int64 account_num, uint16 sector_x, uint16 sector_y);
    void Marshal_ChatRes_Message(uint64 session_id,int64 account_num, WCHAR* id, WCHAR* nick_name, uint16 message_len, WCHAR* message);
    void UnMarshal_ChatReq_Login(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg);
    void UnMarshal_ChatReq_SectorMove(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg);
    void UnMarshal_ChatReq_Message(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg);
    void UnMarshal_ChatReq_HeartBeat(uint64 session_id, SessionInstance* instance, NetSerializeBuffer* msg);
    void Handle_ChatReq_Login(uint64 session_id, SessionInstance* instance, int64 account_num, WCHAR* id, WCHAR* nick_name, char* session_key);
    void Handle_ChatReq_SectorMove(uint64 session_id, SessionInstance* instance, int64 account_num, uint16 sector_x, uint16 sector_y);
    void Handle_ChatReq_Message(uint64 session_id, SessionInstance* instance, int64 account_num, uint16 message_len, WCHAR* message);
    void Handle_ChatReq_HeartBeat(uint64 session_id, SessionInstance* instance);


	//流立 备泅
	void MarshalResMessage(uint64 session_id, uint16 type, int64 account_num, WCHAR* id, WCHAR* nick_name, uint16 message_len, WCHAR* message, uint16 sector_x, uint16 sector_y);
	void HandlePlayerCreate(uint64 session_id, bool success);
	void HandleSectorInfo();
	void MarshalResLoginDisconnect(uint64 session_id, uint16 type, uint8 status, int64 account_num);

public:

	bool EnqueueJob(ChatGroupJob chat_job)
	{
		long job_queue_size = _update_job_queue.Enqueue(chat_job);

		if (job_queue_size == _update_job_queue.MaxQueueSize())
		{
			g_chatserver->OnError(dfLOG_LEVEL_SYSTEM, df_ERROR_REDIS_JOB_QUEUE_FULL, L"Redis Job Queue Full");
			__debugbreak();
		}

		return true;
	}

	int32 UpdateJobQueueSize()
	{
		return _update_job_queue.Size();
	}

	int32 UpdateJobQueueMaxSize()
	{
		return _update_job_queue.MaxQueueSize();
	}

	uint32 GetPlayerPoolCapacity()
	{
		return _player_pool.GetCapacity();
	}

	uint32 GetPlayerPoolUseCnt()
	{
		return _player_pool.GetUseCount();
	}

	uint32 GetPlayerPoolReleaseCnt()
	{
		return _player_pool.GetReleaseCount();
	}

private:

	void CheckSessionKey(int64 account_num, uint64 session_id, char* session_key);

	__forceinline void SendPacketSector(int sector_x, int sector_y, NetSerializeBuffer* msg)
	{
		uint64 session_id;

		for (int dir = 0; dir < 9; dir++)
		{
			int nx = sector_x + dx[dir];
			int ny = sector_y + dy[dir];


			if (nx < 0 || ny < 0 || nx >= MAX_SECTOR_SIZE_X || ny >= MAX_SECTOR_SIZE_Y)
				continue;

			for (auto iter = _sectors[ny][nx].begin(); iter != _sectors[ny][nx].end(); iter++)
			{
				session_id = (*iter)->GetSessionID();
				_service->SendPacketGroup(session_id, msg);
			}
		}
	}

	__forceinline Player* GetPlayer(uint64 session_id)
	{
		auto player = _players.find(session_id);

		if (player == _players.end())
			return nullptr;

		return player->second;
	}

private:
	unordered_map<uint64, Player*> _players;
	ObjectPool<Player, false> _player_pool;
	LockFreeQueue<ChatGroupJob> _update_job_queue;
	list<Player*> _sectors[MAX_SECTOR_SIZE_Y][MAX_SECTOR_SIZE_X];

};