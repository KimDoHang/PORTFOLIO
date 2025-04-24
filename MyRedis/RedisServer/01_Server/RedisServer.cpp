#include "pch.h"
#include "RedisServer.h"
#include "SmartPointer.h"
#include "LanSerializeBuffer.h"
#include "Lock.h"
#include "TextParser.h"
#include "LogUtils.h"
#include "PerformanceMonitor.h"
#include "ThreadManager.h"
#include "Pdh.h"
#include "RedisServer.h"



RedisServer::RedisServer()
{
	_redis_val_pool = new LockFreeObjectPoolTLS<RedisVal, true>;
}

RedisServer::~RedisServer()
{
	delete _redis_val_pool;
}

void RedisServer::RedisInit(const WCHAR* text_file)
{
	TextParser parser;

	bool textfile_load_ret = parser.LoadFile(text_file);

	if (textfile_load_ret == false)
		__debugbreak();

	WCHAR ip[50];
	WCHAR log_level[50];

	int32 port;
	int32 concurrent_thread_num;
	int32 max_thread_num;
	int32 max_player_num;
	int32 max_session_num;

	int32 accept_job;
	bool nagle_onoff = true;

	parser.GetString(L"BIND_IP", ip);
	parser.GetValue(L"BIND_PORT", &port);

	parser.GetValue(L"IOCP_WORKER_THREAD", &max_thread_num);
	parser.GetValue(L"IOCP_ACTIVE_THREAD", &concurrent_thread_num);
	parser.GetValue(L"SESSION_MAX", &max_session_num);
	parser.GetValue(L"USER_MAX", &max_player_num);
	parser.GetValue(L"ACCEPT_JOB", &accept_job);

	parser.GetString(L"LOG_LEVEL", log_level);

	if (wcscmp(log_level, L"DEBUG") == 0)
	{
		g_logutils->SetLogLevel(dfLOG_LEVEL_DEBUG);
	}
	else if (wcscmp(log_level, L"ERROR") == 0)
	{
		g_logutils->SetLogLevel(dfLOG_LEVEL_ERROR);
	}
	else
	{
		g_logutils->SetLogLevel(dfLOG_LEVEL_SYSTEM);
	}

	_set_num = 0;
	_del_num = 0;
	_get_num = 0;

	_print_set_num = df_REDIS_PRINT_JOB_NUM;
	_print_del_num = df_REDIS_PRINT_JOB_NUM;
	_print_get_num = df_REDIS_PRINT_JOB_NUM;

	//_thread_manager->Launch(ExpireThread, this, ServiceType::LanServerType, MonitorInfoType::NoneInfo);

	Init(ip, port, concurrent_thread_num, max_thread_num, true, false, max_session_num, accept_job, 0, 1000);
	LogServerInfo(dfLOG_LEVEL_SYSTEM, L"Redis Server Start");
}

bool RedisServer::OnAccept(uint64 session_id)
{

	WCHAR log_buf[REDIS_LOG_BUF_SIZE];
	wsprintf(log_buf, L"SessionID: %I64u | Redis Login", session_id);
	LogServerInfo(dfLOG_LEVEL_SYSTEM, log_buf);
	return true;
}

void RedisServer::OnRelease(uint64 session_id)
{
	WCHAR log_buf[REDIS_LOG_BUF_SIZE];
	wsprintf(log_buf, L"SessionID: %I64u | Redis Logout", session_id);
	LogServerInfo(dfLOG_LEVEL_SYSTEM, log_buf);
}

void RedisServer::OnRecvMsg(uint64 session_id, LanSerializeBuffer* packet)
{
	WORD type = UINT16_MAX;

	try
	{
		*packet >> type;

		switch (type)
		{
		case en_PACKET_CS_REDIS_REQ_SET:
			UnmarshalRedisSetReq(session_id, packet);
			break;
		case en_PACKET_CS_REDIS_REQ_SET_EXPIRE:
			UnmarshalRedisSetExpireReq(session_id, packet);
			break;
		case en_PACKET_CS_REDIS_REQ_GET:
			UnmarshalRedisGetReq(session_id, packet);
			break;
		case en_PACKET_CS_REDIS_REQ_DEL:
			UnmarshalRedisDelReq(session_id, packet);
			break;
		default:
			OnError(dfLOG_LEVEL_SYSTEM, session_id, type, L"RedisServer MSG Type Error");
			Disconnect(session_id);
			return;
		}

	}
	catch (const NetMsgException& net_msg_exception)
	{
		OnError(dfLOG_LEVEL_SYSTEM, session_id, type, net_msg_exception.whatW());
		Disconnect(session_id);
	}
}

void RedisServer::OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause)
{
	WCHAR log_buff[df_LOG_BUFF_SIZE];

	if (g_logutils->GetLogLevel() <= log_level)
	{
		wsprintfW(log_buff, L"%s [ErrCode:%d] [SessionID:%I64u]", cause, err_code, session_id);
		g_logutils->Log(NET_LOG_DIR, L"RedisServer", log_buff);
	}
}

void RedisServer::OnError(const int8 log_level, int32 err_code, const WCHAR* cause)
{
	WCHAR log_buff[df_LOG_BUFF_SIZE];

	if (g_logutils->GetLogLevel() <= log_level)
	{
		wsprintfW(log_buff, L"%s [ErrCode:%d]", cause, err_code);
		g_logutils->Log(NET_LOG_DIR, L"RedisServer", log_buff);
	}
}

void RedisServer::LogServerInfo(const int8 log_level, const WCHAR* cause)
{

	if (g_logutils->GetLogLevel() <= log_level)
	{
		g_logutils->Log(NET_LOG_DIR, L"RedisServer", cause);
	}
}

bool RedisServer::OnConnectionRequest(WCHAR* IP, SHORT port)
{
	return true;
}

bool RedisServer::OnTimeOut(uint64 session_id)
{
	return true;
}

void RedisServer::OnMonitor()
{
	if (_set_num >= _print_set_num)
	{
		cout << "Set Count " << _set_num << '\n';
		_print_set_num += df_REDIS_PRINT_JOB_NUM;
	}

	if (_get_num >= _print_get_num)
	{
		cout << "Get Count " << _get_num << '\n';
		_print_get_num += df_REDIS_PRINT_JOB_NUM;
	}

	if (_del_num >= _print_del_num)
	{
		cout << "Del Count " << _del_num << '\n';
		_print_del_num += df_REDIS_PRINT_JOB_NUM;
	}
}

void RedisServer::OnExit()
{

}

unsigned int RedisServer::ExpireThread(void* service_ptr)
{
	RedisServer* server = reinterpret_cast<RedisServer*>(service_ptr);
	server->ExpireLoop();
	return 0;
}

void RedisServer::HandleRedisSetReq(uint64 session_id, uint16 server_type, int64 key, RedisVal* rd_val)
{

	switch (server_type)
	{
	case RedisChatServer:
		SetRedisVal(RedisChatServer, key, rd_val);
		break;
	case RedisEchoServer:
		SetRedisVal(RedisEchoServer, key, rd_val);
		break;
	case RedisServerAll:
		for (int type = 0; type < RedisServerType::RedisServerAll; type++)
		{
			SetRedisVal(type, key, rd_val);
		}
		break;
	default:
		OnError(dfLOG_LEVEL_SYSTEM, session_id, server_type, L"RedisServer Server Type Error");
		Disconnect(session_id);
		return;
	}

	//cout << "Set FIN!!! " << "ServerType: " << server_type << " Key: " << key << " Val: " << rd_val->redis_val << '\n';

	AtomicIncrement64(&_set_num);

	MarshalRedisSetRes(session_id, en_PACKET_CS_REDIS_RES_SET, 1);
}

void RedisServer::HandleRedisSetExpireReq(uint64 session_id, uint16 server_type, int64 key, RedisVal* rd_val)
{
	switch (server_type)
	{
	case RedisChatServer:
		SetRedisVal(RedisChatServer, key, rd_val);
		break;
	case RedisEchoServer:
		SetRedisVal(RedisEchoServer, key, rd_val);
		break;
	case RedisServerAll:
		for (int type = 0; type < RedisServerType::RedisServerAll; type++)
		{
			SetRedisVal(type, key, rd_val);
		}
		break;
	default:
		OnError(dfLOG_LEVEL_SYSTEM, session_id, server_type, L"RedisServer Server Type Error");
		Disconnect(session_id);
		return;
	}

	AtomicIncrement64(&_set_num);
	//cout << "SetExpire FIN!!! " << "ServerType: " << server_type << " Key: " << key << " Val: " << rd_val->redis_val << " ExpireTime: " << rd_val->expire_time << '\n';
	MarshalRedisSetExpireRes(session_id, en_PACKET_CS_REDIS_RES_SET_EXPIRE, 1);
}

void RedisServer::HandleRedisGetReq(uint64 session_id, uint16 server_type, int64 key)
{
	int32 hash_idx = key % REDIS_HASH_NUM;
	RedisHash* redis_hash = &_redis_server_containers[server_type][hash_idx];

	char val[REDIS_VAL_MAX_SIZE];
	uint16 val_len;
	uint8 status;

	{
		SrwLockSharedGuard lock_guard(&redis_hash->_redis_val_hash_srwlock);
		
		auto hash_iter = redis_hash->_redis_val_hash.find(key);

		if (hash_iter != redis_hash->_redis_val_hash.end())
		{
			strcpy_s(val, REDIS_VAL_MAX_SIZE, hash_iter->second->redis_val);
			val_len = (uint16)strlen(val);
			status = 1;
		}
		else
		{
			val_len = 0;
			status = 0;
		}

	}

	AtomicIncrement64(&_get_num);
	//cout << "Get FIN!!! " << "ServerType: " << server_type << " Key: " << key << " Val: " << val << '\n';
	MarshalRedisGetRes(session_id, en_PACKET_CS_REDIS_RES_GET, status, val_len, val);
}

void RedisServer::HandleRedisDelReq(uint64 session_id, uint16 server_type, int64 key)
{
	int32 hash_idx = key % REDIS_HASH_NUM;
	RedisHash* redis_hash = &_redis_server_containers[server_type][hash_idx];

	{
		SrwLockSharedGuard lock_guard(&redis_hash->_redis_val_hash_srwlock);
		auto hash_iter = redis_hash->_redis_val_hash.find(key);

		if (hash_iter != redis_hash->_redis_val_hash.end())
		{
			_redis_val_pool->Free(hash_iter->second);
			redis_hash->_redis_val_hash.erase(hash_iter);
		}
	}


	AtomicIncrement64(&_del_num);
	//cout << "Del FIN!!! " << "ServerType: " << server_type << " Key: " << key << '\n';
	MarshalRedisDelRes(session_id, en_PACKET_CS_REDIS_RES_SET_EXPIRE, 1);
}


void RedisServer::UnmarshalRedisSetReq(uint64 session_id, LanSerializeBuffer* msg)
{
	uint16 server_type;
	int64 key;
	uint16 val_len;
	RedisVal* rd_val = _redis_val_pool->Alloc();

	*(msg) >> server_type;
	*(msg) >> key;
	*(msg) >> val_len;
	msg->GetData(rd_val->redis_val, val_len);
	rd_val->redis_val[val_len] = '\0';

	HandleRedisSetReq(session_id, server_type, key, rd_val);
}

void RedisServer::UnmarshalRedisSetExpireReq(uint64 session_id, LanSerializeBuffer* msg)
{
	uint16 server_type;
	int64 key;
	uint16 val_len;
	RedisVal* rd_val = _redis_val_pool->Alloc();

	*(msg) >> server_type;
	*(msg) >> key;
	*(msg) >> rd_val->expire_time;
	rd_val->cur_time = timeGetTime();
	*(msg) >> val_len;
	msg->GetData(rd_val->redis_val, val_len);
	rd_val->redis_val[val_len] = '\0';

	HandleRedisSetExpireReq(session_id, server_type, key, rd_val);
}

void RedisServer::UnmarshalRedisGetReq(uint64 session_id, LanSerializeBuffer* msg)
{
	uint16 server_type;
	int64 key;

	*(msg) >> server_type;
	*(msg) >> key;

	HandleRedisGetReq(session_id, server_type, key);
}

void RedisServer::UnmarshalRedisDelReq(uint64 session_id, LanSerializeBuffer* msg)
{
	uint16 server_type;
	int64 key;

	*(msg) >> server_type;
	*(msg) >> key;

	HandleRedisDelReq(session_id, server_type, key);
}

void RedisServer::MarshalRedisSetRes(uint64 session_id, uint16 type, uint8 status)
{
	LanSerializeBuffer* msg = ALLOC_LAN_PACKET();
	*(msg) << type << status;
	SendPacket(session_id, msg);
	FREE_LAN_SEND_PACKET(msg);
}

void RedisServer::MarshalRedisSetExpireRes(uint64 session_id, uint16 type, uint8 status)
{
	LanSerializeBuffer* msg = ALLOC_LAN_PACKET();
	*(msg) << type << status;
	SendPacket(session_id, msg);
	FREE_LAN_SEND_PACKET(msg);
}

void RedisServer::MarshalRedisGetRes(uint64 session_id, uint16 type, uint8 status, uint16 val_len, char* val)
{
	LanSerializeBuffer* msg = ALLOC_LAN_PACKET();
	*(msg) << type << status << val_len;
	msg->PutData(val, val_len);
	SendPacket(session_id, msg);
	FREE_LAN_SEND_PACKET(msg);

}

void RedisServer::MarshalRedisDelRes(uint64 session_id, uint16 type, uint8 status)
{
	LanSerializeBuffer* msg = ALLOC_LAN_PACKET();
	*(msg) << type << status;
	SendPacket(session_id, msg);
	FREE_LAN_SEND_PACKET(msg);
}

void RedisServer::SetRedisVal(uint16 server_type, int64 key, RedisVal* rd_val)
{
	int32 hash_idx = key % REDIS_HASH_NUM;

	RedisHash* redis_hash = &_redis_server_containers[server_type][hash_idx];

	{
		SrwLockExclusiveGuard lock_guard(&redis_hash->_redis_val_hash_srwlock);

		auto hash_iter = redis_hash->_redis_val_hash.find(key);

		if (hash_iter != redis_hash->_redis_val_hash.end())
		{
			_redis_val_pool->Free(hash_iter->second);
			redis_hash->_redis_val_hash.erase(hash_iter);
		}
		redis_hash->_redis_val_hash[key] = rd_val;
	}
}


