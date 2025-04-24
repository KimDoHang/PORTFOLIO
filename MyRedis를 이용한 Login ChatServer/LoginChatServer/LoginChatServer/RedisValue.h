#pragma once
#include "pch.h"
#include "LockFreeObjectPoolTLS.h"


//RedisArrServer Values
#define REDIS_ARR_SIZE 1000
#define REDIS_MAX_SESSION_IDS 10000

//RedisHashServer Values
#define REDIS_HASH_NUM 100

//RedisServer Common Values
#define REDIS_SERVER_TYPE_MAX 2
#define REDIS_KEY_MAX_SIZE 128
#define REDIS_VAL_MAX_SIZE 128
#define REDIS_LOG_BUF_SIZE 256

#define df_REDIS_PRINT_JOB_NUM 10000

enum RedisJobType
{
	RedisSetJob = 1,
	RedisSetExpireJob = 2,
	RedisGetJob = 3,
	RedisDelJob = 4,
	RedisExitJob = 5,
};


enum RedisServerType
{
	RedisChatServer = 0,
	RedisEchoServer = 1,
	RedisServerAll = 2,
};

struct RedisVal
{
	RedisVal()
	{
		expire_time = 0;
		cur_time = 0;
	}

	char redis_val[REDIS_VAL_MAX_SIZE];
	int32 expire_time;
	uint32 cur_time;
};

struct RedisRet
{
	RedisRet() {}

	RedisRet(const RedisRet& obj)
	{
		_status = obj._status;
		strcpy_s(_val, obj._val);
	}
	uint8 _status;
	char _val[REDIS_VAL_MAX_SIZE];
};

struct RedisArr
{

	RedisArr()
	{
		for (int i = 0; i < REDIS_ARR_SIZE; i++)
		{
			_redis_val_arr[i] = nullptr;
		}
		InitializeSRWLock(&_redis_val_arr_srwlock);
	}

	SRWLOCK _redis_val_arr_srwlock;
	RedisVal* _redis_val_arr[REDIS_ARR_SIZE];
};

struct RedisHash
{
	RedisHash()
	{
		InitializeSRWLock(&_redis_val_hash_srwlock);
	}

	SRWLOCK _redis_val_hash_srwlock;
	unordered_map<int64, RedisVal*> _redis_val_hash;
};