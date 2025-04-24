#pragma once
#include "LanService.h"
#include "LockFreeQueue.h"
#include "Values.h"
#include "LanSendPacket.h"
#include "Lock.h"
#include <unordered_map>

class RedisArrServer : public LanService
{
public:
	RedisArrServer();
	~RedisArrServer();

	void RedisInit(const WCHAR* text_file);
	virtual bool OnAccept(uint64 session_id);
	virtual void OnRelease(uint64 session_id);
	virtual void OnRecvMsg(uint64 session_id, LanSerializeBuffer* packet);
	virtual void OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause);
	virtual void OnError(const int8 log_level, int32 err_code, const WCHAR* cause);
	void LogServer(const int8 log_level, const WCHAR* cause);

	virtual bool OnConnectionRequest(WCHAR* IP, SHORT port);
	virtual bool OnTimeOut(uint64 session_id);
	virtual void OnMonitor();
	virtual void OnExit();


	void ExpireLoop()
	{
		cur_tick = timeGetTime();

		//	cout << "TimeOut Time : " << cur_tick - past_tick << '\n';

		for (int server_type = 0; server_type < REDIS_SERVER_TYPE_MAX; server_type++)
		{
			for (int arr_idx = 0; arr_idx < REDIS_MAX_SESSION_IDS / REDIS_ARR_SIZE; arr_idx++)
			{
				RedisArr* rd_arr = &_redis_server_containers[server_type][arr_idx];
				SrwLockExclusiveGuard lock_guard(&rd_arr->_redis_val_arr_srwlock);

				for (int idx = 0; idx < REDIS_ARR_SIZE; idx++)
				{
					RedisVal* rd_val = rd_arr->_redis_val_arr[idx];

					if (rd_val == nullptr)
						continue;

					if (rd_val->expire_time == 0)
						continue;

					if (cur_tick > (uint32)(rd_val->cur_time + rd_val->expire_time * 1000))
					{
						cout << "Delete!!! Val: " << rd_val->redis_val << " CurTick: " << cur_tick << " LastTick: " << rd_val->cur_time << " ExpireTick: " << rd_val->expire_time << '\n';
						_redis_val_pool->Free(rd_val);
						rd_arr->_redis_val_arr[idx] = nullptr;
					}
				}

			}
		}

		past_tick = cur_tick;
	}

private:
	void HandleRedisSetReq(uint64 session_id, uint16 server_type, int64 key, RedisVal* rd_val);
	void HandleRedisSetExpireReq(uint64 session_id, uint16 server_type, int64 key, RedisVal* rd_val);
	void HandleRedisGetReq(uint64 session_id, uint16 server_type, int64 key);
	void HandleRedisDelReq(uint64 session_id, uint16 server_type, int64 key);

	void UnmarshalRedisSetReq(uint64 session_id, LanSerializeBuffer* msg);
	void UnmarshalRedisSetExpireReq(uint64 session_id, LanSerializeBuffer* msg);
	void UnmarshalRedisGetReq(uint64 session_id, LanSerializeBuffer* msg);
	void UnmarshalRedisDelReq(uint64 session_id, LanSerializeBuffer* msg);


	void MarshalRedisSetRes(uint64 session_id, uint16 type, uint8 status);
	void MarshalRedisSetExpireRes(uint64 session_id, uint16 type, uint8 status);
	void MarshalRedisGetRes(uint64 session_id, uint16 type, uint8 status, uint16 val_len, char* val);
	void MarshalRedisDelRes(uint64 session_id, uint16 type, uint8 status);

	void SetRedisVal(uint16 server_type, int64 key, RedisVal* rd_val)
	{
		int32 containers_idx = (int32)(key / REDIS_ARR_SIZE);
		int32 arr_idx = key % REDIS_ARR_SIZE;

		RedisArr* redis_arr = &_redis_server_containers[server_type][containers_idx];

		{
			SrwLockExclusiveGuard lock_guard(&redis_arr->_redis_val_arr_srwlock);
			if (redis_arr->_redis_val_arr[arr_idx] != nullptr)
			{
				_redis_val_pool->Free(redis_arr->_redis_val_arr[arr_idx]);
			}

			redis_arr->_redis_val_arr[arr_idx] = rd_val;
		}
	}

private:
	
	uint64 _set_num;
	uint64 _del_num;
	uint64 _get_num;

private:
	uint32 cur_tick;
	uint32 past_tick;
	int32 _cnt_msg;
	LockFreeObjectPoolTLS<RedisVal, true>* _redis_val_pool;
	RedisArr _redis_server_containers[REDIS_SERVER_TYPE_MAX][REDIS_MAX_SESSION_IDS / REDIS_ARR_SIZE];
};
