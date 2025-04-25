#include "pch.h"
#include "RedisUtils.h"

thread_local RedisClient* RedisUtils::_redis_client = nullptr;


unsigned int RedisUtils::RedisThread(void* redis_ptr)
{
	RedisUtils* redis = (RedisUtils*)redis_ptr;

	redis->ReidsAsyncThreadLoop();

	return 0;
}

void RedisUtils::StartRedisThread()
{
	if (_redis_thread_handle != INVALID_HANDLE_VALUE)
		__debugbreak();

	_redis_thread_handle = (HANDLE)_beginthreadex(nullptr, 0, RedisUtils::RedisThread, (void*)this, 0, nullptr);
}

bool RedisUtils::ClearRedisThread()
{
	RedisJob* job = _job_pool->Alloc();
	job->JobInit(RedisJobType::RedisSetJob);

	long job_cnt = _job_queue->Enqueue(job);

	if (job_cnt == REDIS_JOB_QUEUE_SIZE)
	{
		_job_pool->Free(job);
		return false;
	}

	if (job_cnt == 1)
	{
		WakeByAddressSingle(_job_queue->SizePtr());
	}


	WaitForSingleObject(_redis_thread_handle, INFINITE);
	CloseHandle(_redis_thread_handle);
	_redis_thread_handle = INVALID_HANDLE_VALUE;
	return true;
}



void RedisUtils::ReidsAsyncThreadLoop()
{
	InitRedisClient();

	long size;
	RedisJob* job;

	RedisClient* redis_client = _redis_client;

	while (true)
	{

		while (true)
		{
			size = _job_queue->Size();

			if (size == 0)
				break;

			_job_queue->Dequeue(job);

			switch (job->_type)
			{
			case RedisJobType::RedisSetJob:
			{
				RedisRet* reply_val = redis_client->Set(job->_server_type, job->_key, job->_val.c_str());
				redis_client->SyncCommit();
				job->_callback_functor(*reply_val, job->_callback_data);
			}
			break;
			case RedisJobType::RedisSetExpireJob:
			{
				RedisRet* reply_val = redis_client->SetExpire(job->_server_type, job->_key, job->_expire, job->_val.c_str());
				redis_client->SyncCommit();
				job->_callback_functor(*reply_val, job->_callback_data);
			}
			break;
			case RedisJobType::RedisGetJob:
			{
				RedisRet* reply_val = redis_client->Get(job->_server_type, job->_key);
				redis_client->SyncCommit();
				job->_callback_functor(*reply_val, job->_callback_data);
			}
			break;
			case RedisJobType::RedisExitJob:
				_job_pool->Free(job);
				return;
			default:
				__debugbreak();
				break;
			}

			_job_pool->Free(job);
		}

		WaitOnAddress(_job_queue->SizePtr(), &size, sizeof(size), INFINITE);
	}
}

RedisRet RedisUtils::SetSync(uint16 server_type, int64 key, const char* val)
{
	InitRedisClient();

	RedisRet* reply_val = _redis_client->Set(server_type, key, val);
	_redis_client->SyncCommit();
	return *reply_val;
}

RedisRet RedisUtils::SetSync(uint16 server_type, int64 key, int32 expire, const char* val)
{
	InitRedisClient();


	RedisRet* reply_val = _redis_client->SetExpire(server_type, key, expire, val);
	_redis_client->SyncCommit();
	return *reply_val;
}

RedisRet RedisUtils::GetSync(uint16 server_type, int64 key)
{
	InitRedisClient();


	RedisRet* reply_val = _redis_client->Get(server_type, key);
	_redis_client->SyncCommit();
	return *reply_val;
}

RedisRet RedisUtils::DelSync(uint16 server_type, int64 key)
{
	InitRedisClient();


	RedisRet* reply_val = _redis_client->Del(server_type, key);
	_redis_client->SyncCommit();
	return *reply_val;
}

bool RedisUtils::SetAsync(uint16 server_type, int64 key, string& val, void* callback_data, RedisCallBack callback_functor)
{
	RedisJob* job = _job_pool->Alloc();
	job->JobInit(RedisJobType::RedisSetJob, server_type, key, val, callback_data, callback_functor);

	long job_cnt = _job_queue->Enqueue(job);

	if (job_cnt == REDIS_JOB_QUEUE_SIZE)
	{
		_job_pool->Free(job);
		return false;
	}

	if (job_cnt == 1)
	{
		WakeByAddressSingle(_job_queue->SizePtr());
	}

	return true;
}

bool RedisUtils::SetAsync(uint16 server_type, int64 key, int32 expire, string& val, void* callback_data, RedisCallBack callback_functor)
{

	RedisJob* job = _job_pool->Alloc();

	job->JobInit(RedisJobType::RedisSetExpireJob, server_type, key, expire, val, callback_data, callback_functor);

	long job_cnt = _job_queue->Enqueue(job);

	if (job_cnt == REDIS_JOB_QUEUE_SIZE)
	{
		_job_pool->Free(job);
		return false;
	}

	if (job_cnt == 1)
	{
		WakeByAddressSingle(_job_queue->SizePtr());
	}

	return true;
}

bool RedisUtils::GetAsync(uint16 server_type, int64 key, void* callback_data, RedisCallBack callback_functor)
{
	RedisJob* job = _job_pool->Alloc();
	job->JobInit(RedisJobType::RedisGetJob, server_type, key, callback_data, callback_functor);

	long job_cnt = _job_queue->Enqueue(job);

	if (job_cnt == REDIS_JOB_QUEUE_SIZE)
	{
		_job_pool->Free(job);
		return false;
	}

	if (job_cnt == 1)
	{
		WakeByAddressSingle(_job_queue->SizePtr());
	}

	return true;
}

