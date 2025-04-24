#pragma once

#include "LockFreeQueue.h"
#include "LockFreeObjectPoolTLS.h"
#include "RedisClient.h"
#include "TextParser.h"
//using RedisCallBack = std::function<void(cpp_redis::reply reply, void*)>;
using RedisCallBack = void(*)(RedisRet ret_val, void*);

#define df_ERROR_REDIS_JOB_QUEUE_FULL 30001

class RedisUtils
{
	static const int32 REDIS_JOB_BUFF_SIZE = 200;
	static const int32 REDIS_JOB_QUEUE_SIZE = 100000;
	static const int32 REDIS_IP_BUFF_SIZE = 128;
	struct RedisJob
	{
		RedisJob()
		{

		}

		void JobInit(RedisJobType type)
		{
			_type = type;
		}

		void JobInit(RedisJobType type, uint16 server_type, int64 key, void* callback_data, RedisCallBack callback_functor)
		{
			_type = type;
			_server_type = server_type;
			_key = key;
			_callback_data = callback_data;
			_callback_functor = callback_functor;
		}

		void JobInit(RedisJobType type, uint16 server_type, int64 key, string& val, void* callback_data, RedisCallBack callback_functor)
		{
			_type = type;
			_server_type = server_type;
			_key = key;
			_val = val;
			_callback_data = callback_data;
			_callback_functor = callback_functor;
		}

		void JobInit(RedisJobType type, uint16 server_type, int64 key, int32 expire, string& val, void* callback_data, RedisCallBack callback_functor)
		{
			_type = type;
			_server_type = server_type;
			_key = key;
			_val = val;
			_expire = expire;
			_callback_data = callback_data;
			_callback_functor = callback_functor;
		}



		void JobInit(RedisJobType type, uint16 server_type, int64 key, char* val, void* callback_data, RedisCallBack callback_functor)
		{
			_type = type;
			_server_type = server_type;
			_key = key;
			_val = val;
			_callback_data = callback_data;
			_callback_functor = callback_functor;
		}

		void JobInit(RedisJobType type, uint16 server_type, int64 key, char* val, int32 expire, void* callback_data, RedisCallBack callback_functor)
		{
			_type = type;
			_server_type = server_type;
			_key = key;
			_val = val;
			_expire = expire;
			_callback_data = callback_data;
			_callback_functor = callback_functor;
		}

		RedisJobType _type;
		uint16 _server_type;
		int64 _key;
		int32 _expire;
		string _val;
		void* _callback_data;
		RedisCallBack _callback_functor;
	};


public:
	RedisUtils(const WCHAR* ip, uint16 port, int32 concurrent_thread_nums = 1, int32 max_thread_nums = 1) : _redis_port(port), _redis_thread_handle(INVALID_HANDLE_VALUE)
	{
		wcscpy_s(_redis_ip, REDIS_IP_BUFF_SIZE, ip);
		InitializeCriticalSection(&_redis_vec_lock);
		_redis_client_vec.reserve(10);
		_redis_concurrent_thread_nums = concurrent_thread_nums;
		_redis_max_thread_nums = max_thread_nums;

		_job_queue = new LockFreeQueue<RedisJob*, REDIS_JOB_QUEUE_SIZE>;
		_job_pool = new LockFreeObjectPoolTLS<RedisJob, false>;
	}


	RedisUtils(const WCHAR* redis_config) : _redis_thread_handle(INVALID_HANDLE_VALUE), _redis_port(0)
	{

		InitializeCriticalSection(&_redis_vec_lock);
		_redis_client_vec.reserve(10);

		TextParser parser;
		int32 redis_port;
		int32 max_thread_nums;
		int32 concurrent_thread_nums;

		bool textfile_load_ret = parser.LoadFile(redis_config);

		if (textfile_load_ret == false)
			__debugbreak();

		parser.GetString(L"BIND_IP", _redis_ip);
		parser.GetValue(L"BIND_PORT", &redis_port);
		parser.GetValue(L"IOCP_WORKER_THREAD", &max_thread_nums);
		parser.GetValue(L"IOCP_ACTIVE_THREAD", &concurrent_thread_nums);

		_redis_port = redis_port;
		_redis_max_thread_nums = max_thread_nums;
		_redis_concurrent_thread_nums = concurrent_thread_nums;

		_job_queue = new LockFreeQueue<RedisJob*, REDIS_JOB_QUEUE_SIZE>;
		_job_pool = new LockFreeObjectPoolTLS<RedisJob, false>;
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
				//(*iter)->OnExit();
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

	RedisRet SetSync(uint16 server_type, int64 key, const char* val);
	RedisRet SetSync(uint16 server_type, int64 key, int32 expire, const char* val);
	RedisRet GetSync(uint16 server_type, int64 key);
	RedisRet DelSync(uint16 server_type, int64 key);

	bool SetAsync(uint16 server_type, int64 key, string& val, void* callback_data, RedisCallBack callback_functor);
	bool SetAsync(uint16 server_type, int64 key, int32 expire, string& val, void* callback_data, RedisCallBack callback_functor);
	bool GetAsync(uint16 server_type, int64 key, void* callback_data, RedisCallBack callback_functor);

	void SyncCommit() { _redis_client->SyncCommit(); }



	__forceinline void InitRedisClient()
	{
		if (_redis_client == nullptr)
		{

			EnterCriticalSection(&_redis_vec_lock);

			_redis_client = new RedisClient;
			_redis_client_vec.push_back(_redis_client);

			if (_redis_port == 0)
			{
				__debugbreak();
			}

			_redis_client->RedisConnect(_redis_ip, _redis_port, _redis_concurrent_thread_nums, _redis_max_thread_nums);

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
	HANDLE _redis_thread_handle;
	CRITICAL_SECTION _redis_vec_lock;
	uint16 _redis_port;
	int32 _redis_concurrent_thread_nums;
	int32 _redis_max_thread_nums;
	WCHAR _redis_ip[REDIS_IP_BUFF_SIZE];
	vector<RedisClient*> _redis_client_vec;
	LockFreeQueue<RedisJob*, REDIS_JOB_QUEUE_SIZE>* _job_queue;
	LockFreeObjectPoolTLS<RedisJob, false>* _job_pool;
	static thread_local RedisClient* _redis_client;
};


