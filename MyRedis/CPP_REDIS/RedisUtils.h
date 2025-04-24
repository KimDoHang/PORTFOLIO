#pragma once
#include <cpp_redis/cpp_redis>
#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#pragma comment (lib, "ws2_32.lib")
#include <functional>
#include "LockFreeQueue.h"
#include "LockFreeObjectPoolTLS.h"
#include "TextParser.h"
//using RedisCallBack = std::function<void(cpp_redis::reply reply, void*)>;
using RedisCallBack = void(*)(cpp_redis::reply&& reply, void*);
#define df_ERROR_REDIS_JOB_QUEUE_FULL 30001


enum class RedisJobType : uint8
{
	RedisSetJob = 1,
	RedisSetExpireJob = 2,
	RedisGetJob = 3,
	RedisExitJob,
};


class RedisUtils
{
	static const int32 REDIS_JOB_BUFF_SIZE = 200;
	static const int32 REDIS_JOB_QUEUE_SIZE = 100000;
	static const int32 REDIS_IP_BUF_SIZE = 20;
	struct RedisJob
	{
		RedisJob()
		{

		}
		RedisJob(RedisJobType type)
		{
			_type = type;
		}

		RedisJob(RedisJobType type, string& key, void* callback_data, RedisCallBack callback_functor)
		{
			_type = type;
			_key = key;
			_callback_data = callback_data;
			_callback_functor = callback_functor;
		}

		RedisJob(RedisJobType type, string& key, string& val, void* callback_data, RedisCallBack callback_functor)
		{
			_type = type;
			_key = key;
			_val = val;
			_callback_data = callback_data;
			_callback_functor = callback_functor;
		}

		RedisJob(RedisJobType type, string& key, string& val, string& expire, void* callback_data, RedisCallBack callback_functor)
		{
			_type = type;
			_key = key;
			_val = val;
			_expire = expire;
			_callback_data = callback_data;
			_callback_functor = callback_functor;
		}

		RedisJob(RedisJobType type, char* key, void* callback_data, RedisCallBack callback_functor)
		{
			_type = type;
			_key = key;
			_callback_data = callback_data;
			_callback_functor = callback_functor;
		}

		RedisJob(RedisJobType type, char* key, char* val, void* callback_data, RedisCallBack callback_functor)
		{
			_type = type;
			_key = key;
			_val = val;
			_callback_data = callback_data;
			_callback_functor = callback_functor;
		}

		RedisJob(RedisJobType type, char* key, char* val, char* expire, void* callback_data, RedisCallBack callback_functor)
		{
			_type = type;
			_key = key;
			_val = val;
			_expire = expire;
			_callback_data = callback_data;
			_callback_functor = callback_functor;
		}

		RedisJobType _type;
		string _key;
		string _val;
		string _expire;
		void* _callback_data;
		RedisCallBack _callback_functor;
	};


public:
	RedisUtils(const char* ip = "127.0.0.1", size_t port = 6379) : _redis_port(port), _redis_thread_handle(INVALID_HANDLE_VALUE)
	{
		strcpy_s(_redis_ip, REDIS_IP_BUF_SIZE, ip);

		InitializeCriticalSection(&_redis_vec_lock);
		_redis_client_vec.reserve(10);

		_job_queue = new LockFreeQueue<RedisJob*, REDIS_JOB_QUEUE_SIZE>;
		_job_pool = new LockFreeObjectPoolTLS<RedisJob, true>;
	}


	RedisUtils(const char* redis_config) : _redis_thread_handle(INVALID_HANDLE_VALUE)
	{
		InitializeCriticalSection(&_redis_vec_lock);
		_redis_client_vec.reserve(10);

		TextParser parser;
		int32 redis_port;

		bool textfile_load_ret = parser.LoadFile(redis_config);

		if (textfile_load_ret == false)
			__debugbreak();

		parser.GetString("BIND_IP", _redis_ip);
		parser.GetValue("BIND_PORT", &redis_port);

		_redis_port = redis_port;

		_job_queue = new LockFreeQueue<RedisJob*, REDIS_JOB_QUEUE_SIZE>;
		_job_pool = new LockFreeObjectPoolTLS<RedisJob, true>;
	}

	~RedisUtils()
	{

		if (_redis_thread_handle != INVALID_HANDLE_VALUE)
		{
			ClearRedisThread();
		}

		{
			EnterCriticalSection(&_redis_vec_lock);

			for (auto iter = _redis_client_vec.begin(); iter != _redis_client_vec.end(); iter++)
			{
				(*iter)->disconnect();
				delete* iter;
			}

			LeaveCriticalSection(&_redis_vec_lock);
		}

		delete _job_queue;
		delete _job_pool;

		DeleteCriticalSection(&_redis_vec_lock);
	}

	static unsigned int RedisThread(void* redis);

	void StartRedisThread();
	bool ClearRedisThread();

	void ReidsAsyncThreadLoop();

	cpp_redis::reply SetSync(const char* key, const char* val);
	cpp_redis::reply SetSync(const char* key, const char* val, const char* expire);
	cpp_redis::reply GetSync(char* key);
	cpp_redis::reply DelSync(char* key);

	bool SetAsync(string& key, string& val, void* callback_data, RedisCallBack callback_functor);
	bool SetAsync(string& key, string& val, string& expire, void* callback_data, RedisCallBack callback_functor);
	bool GetAsync(char* key, void* callback_data, RedisCallBack callback_functor);

	__forceinline void InitRedisClient()
	{
		if (_redis_client == nullptr)
		{

			EnterCriticalSection(&_redis_vec_lock);

			_redis_client = new cpp_redis::client;
			_redis_client_vec.push_back(_redis_client);

			if (_redis_port == 0)
			{
				__debugbreak();
			}

			_redis_client->connect(_redis_ip, _redis_port);

			LeaveCriticalSection(&_redis_vec_lock);

		}
	}

	int32 GetJobQueueSize()
	{
		return _job_queue->Size();
	}

	int32 GetJobQueueMaxSize()
	{
		return _job_queue->MaxQueueSize();
	}

	__forceinline uint32 GetJobChunkCapacityCount()
	{
		return _job_pool->GetChunkCapacityCount();
	}

	__forceinline uint32 GetJobChunkUseCount()
	{
		return _job_pool->GetChunkNodeUseCount();
	}

	__forceinline uint32 GetJobChunkReleaseCount()
	{
		return _job_pool->GetChunkReleaseCount();
	}

	__forceinline uint32 GetJobChunkNodeUseCount()
	{
		return _job_pool->GetChunkNodeUseCount();
	}

	__forceinline uint32 GetJobChunkNodeReleaseCount()
	{
		return _job_pool->GetChunkNodeReleaseCount();
	}


private:
	//Redis 시작 IP, PORT
	char _redis_ip[REDIS_IP_BUF_SIZE];
	size_t _redis_port;
	//동기, 비동기 Thread별 Redis client 관리 Data
	vector<cpp_redis::client*> _redis_client_vec;
	CRITICAL_SECTION _redis_vec_lock;

	//비동기 Redis HANDLE
	HANDLE _redis_thread_handle;
	LockFreeQueue<RedisJob*, REDIS_JOB_QUEUE_SIZE>* _job_queue;
	LockFreeObjectPoolTLS<RedisJob, true>* _job_pool;

	static thread_local cpp_redis::client* _redis_client;
};


