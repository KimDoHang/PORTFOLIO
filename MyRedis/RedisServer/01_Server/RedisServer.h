#pragma once
#include "LanService.h"
#include "LockFreeQueue.h"
#include "Values.h"
#include "LanSendPacket.h"
#include "Lock.h"
#include <unordered_map>
#include "ThreadManager.h"
class RedisServer : public LanService
{
public:
	RedisServer();
	~RedisServer();

	void RedisInit(const WCHAR* text_file);
	virtual bool OnAccept(uint64 session_id);
	virtual void OnRelease(uint64 session_id);
	virtual void OnRecvMsg(uint64 session_id, LanSerializeBuffer* packet);
	virtual void OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause);
	virtual void OnError(const int8 log_level, int32 err_code, const WCHAR* cause);
	void LogServerInfo(const int8 log_level, const WCHAR* cause);

	virtual bool OnConnectionRequest(WCHAR* IP, SHORT port);
	virtual bool OnTimeOut(uint64 session_id);
	virtual void OnMonitor();
	virtual void OnExit();

	static unsigned int ExpireThread(void* service_ptr);

	void ExpireLoop()
	{
		_thread_manager->Wait();


		while (GetExitFlag() == 0)
		{

			for (int server_type = 0; server_type < REDIS_SERVER_TYPE_MAX; server_type++)
			{
				for (int hash_idx = 0; hash_idx < REDIS_HASH_NUM; hash_idx++)
				{
					RedisHash* rd_hash = &_redis_server_containers[server_type][hash_idx];
					SrwLockExclusiveGuard lock_guard(&rd_hash->_redis_val_hash_srwlock);

					for (auto hash_iter = rd_hash->_redis_val_hash.begin(); hash_iter != rd_hash->_redis_val_hash.end();)
					{
						RedisVal* rd_val = hash_iter->second;

						if (rd_val->expire_time == 0)
						{
							hash_iter++;
							continue;
						}

						//if (cur_tick > (uint32)(rd_val->cur_time + rd_val->expire_time * 1000))
						//{
						//	cout << "Delete!!! Val: " << rd_val->redis_val << " CurTick: " << cur_tick << " LastTick: " << rd_val->cur_time << " ExpireTick: " << rd_val->expire_time << '\n';
						//	_redis_val_pool->Free(rd_val);
						//}
					}

				}
			}

			Sleep(1000);

		}
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

	void SetRedisVal(uint16 server_type, int64 key, RedisVal* rd_val);

private:
	uint64 _set_num;
	uint64 _del_num;
	uint64 _get_num;

	uint64 _print_set_num;
	uint64 _print_del_num;
	uint64 _print_get_num;
	
private:
	int32 _cnt_msg;
	LockFreeObjectPoolTLS<RedisVal, true>* _redis_val_pool;
	RedisHash _redis_server_containers[REDIS_SERVER_TYPE_MAX][REDIS_HASH_NUM];
};
