#pragma once
#include "LanClient.h"
#include "LockFreeQueue.h"
#include "LanSerializeBuffer.h"
#include "RedisValue.h"

class RedisClient : public LanClient
{
public:

	static const int32 MAX_SYNC_TIME_OUT = 600000;

	RedisClient()
	{
		_sync_event = CreateEvent(nullptr, true, false, nullptr);
		_redis_callback_data_queue = new LockFreeQueue<void*>;
	}

	~RedisClient()
	{
		CloseHandle(_sync_event);
		delete _redis_callback_data_queue;
	}

	bool RedisConnect(WCHAR* ip, uint16 port, int32 concurrent_thread_nums, int32 max_thread_nums);

	void SyncCommit()
	{
		DWORD ret = WaitForSingleObject(_sync_event, MAX_SYNC_TIME_OUT);
		if (ret == WAIT_TIMEOUT)
		{
			__debugbreak();
		}

	}

	RedisRet* Set(uint16 server_type, int64 key, const char* val);
	RedisRet* SetExpire(uint16 server_type, int64 key, int32 expire_time, const char* val);
	RedisRet* Get(uint16 server_type, int64 key);
	RedisRet* Del(uint16 server_type, int64 key);

public:
	virtual void OnConnect();
	virtual void OnRelease();
	virtual void OnRecvMsg(LanSerializeBuffer* packet);
	virtual void OnError(const int8 log_level, int32 err_code, const WCHAR* cause);
	virtual void OnExit();


private:

	void HandleRedisLoginRes(uint8 status);
	void HandleRedisSetRes(uint8 status);
	void HandleRedisSetExpireRes(uint8 status);
	void HandleRedisGetRes(uint8 status, uint16 val_len, const char* val);
	void HandleRedisDelRes(uint8 status);

	void UnmarshalRedisLoginRes(LanSerializeBuffer* msg);
	void UnmarshalRedisSetRes(LanSerializeBuffer* msg);
	void UnmarshalRedisSetExpireRes(LanSerializeBuffer* msg);
	void UnmarshalRedisGetRes(LanSerializeBuffer* msg);
	void UnmarshalRedisDelRes(LanSerializeBuffer* msg);

private:
	HANDLE _sync_event;
	RedisRet _redis_ret;
	LockFreeQueue<void*>* _redis_callback_data_queue;
};

