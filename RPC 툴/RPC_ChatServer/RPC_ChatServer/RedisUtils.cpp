#include "pch.h"
#include "RedisUtils.h"

thread_local cpp_redis::client* RedisUtils::_redis_client = nullptr;


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
	RedisJob* job = _job_pool->Alloc(RedisJobType::RedisExitJob);

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

	cpp_redis::client* redis_client = _redis_client;

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
				future<cpp_redis::reply>&& reply_val = redis_client->set(job->_key, job->_val);
				redis_client->sync_commit();
				job->_callback_functor(reply_val.get(), job->_callback_data);
			}
			break;
			case RedisJobType::RedisSetExpireJob:
			{
				future<cpp_redis::reply>&& reply_val = redis_client->send({ "SET", job->_key, job->_val, "EX", job->_expire });
				redis_client->sync_commit();
				job->_callback_functor(reply_val.get(), job->_callback_data);
			}
			break;
			case RedisJobType::RedisGetJob:
			{
				future<cpp_redis::reply>&& reply_val = redis_client->get(job->_key);
				redis_client->sync_commit();
				job->_callback_functor(reply_val.get(), job->_callback_data);
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

cpp_redis::reply RedisUtils::SetSync(const char* key, const char* val)
{
	InitRedisClient();

	future<cpp_redis::reply>&& reply_val = _redis_client->set(key, val);
	_redis_client->sync_commit();
	return reply_val.get();
}

cpp_redis::reply RedisUtils::SetSync(const char* key, const char* val, const char* expire)
{
	InitRedisClient();

	future<cpp_redis::reply>&& reply_val = _redis_client->send({ "SET", key, val, "EX", expire });
	_redis_client->sync_commit();
	return reply_val.get();
}

cpp_redis::reply RedisUtils::GetSync(char* key)
{
	InitRedisClient();

	future<cpp_redis::reply>&& reply_val = _redis_client->get(key);
	_redis_client->sync_commit();

	return reply_val.get();
}

cpp_redis::reply RedisUtils::DelSync(char* key)
{
	InitRedisClient();

	future<cpp_redis::reply>&& reply_val = _redis_client->del({ key });
	_redis_client->sync_commit();

	return reply_val.get();
}

bool RedisUtils::SetAsync(string& key, string& val, void* callback_data, RedisCallBack callback_functor)
{
	RedisJob* job = _job_pool->Alloc(RedisJobType::RedisSetJob, key, val, callback_data, callback_functor);


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

bool RedisUtils::SetAsync(string& key, string& val, string& expire, void* callback_data, RedisCallBack callback_functor)
{
	RedisJob* job = _job_pool->Alloc(RedisJobType::RedisSetExpireJob, key, val, expire, callback_data, callback_functor);

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

bool RedisUtils::GetAsync(char* key, void* callback_data, RedisCallBack callback_functor)
{
	RedisJob* job = _job_pool->Alloc(RedisJobType::RedisGetJob, key, callback_data, callback_functor);

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
