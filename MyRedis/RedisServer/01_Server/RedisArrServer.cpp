#include "pch.h"
#include "RedisArrServer.h"
#include "SmartPointer.h"
#include "LanSerializeBuffer.h"
#include "Lock.h"
#include "TextParser.h"
#include "LogUtils.h"
#include "PerformanceMonitor.h"
#include "ThreadManager.h"
#include "Pdh.h"
#include "RedisArrServer.h"



RedisArrServer::RedisArrServer()
{
	_redis_val_pool = new LockFreeObjectPoolTLS<RedisVal, true>;
}

RedisArrServer::~RedisArrServer()
{
	delete _redis_val_pool;
}

void RedisArrServer::RedisInit(const WCHAR* text_file)
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

	Init(ip, port, concurrent_thread_num, max_thread_num, nagle_onoff, max_session_num, accept_job, 0, 1000, 0);
	LogServer(dfLOG_LEVEL_SYSTEM, L"Redis Server Start");
}

bool RedisArrServer::OnAccept(uint64 session_id)
{

	WCHAR log_buf[REDIS_LOG_BUF_SIZE];
	wsprintf(log_buf, L"SessionID: %d | Redis Login", session_id);
	LogServer(dfLOG_LEVEL_SYSTEM,log_buf);
	return true;
}

void RedisArrServer::OnRelease(uint64 session_id)
{
	WCHAR log_buf[REDIS_LOG_BUF_SIZE];
	wsprintf(log_buf, L"SessionID: %d | Redis Logout", session_id);
	LogServer(dfLOG_LEVEL_SYSTEM, log_buf);
}

void RedisArrServer::OnRecvMsg(uint64 session_id, LanSerializeBuffer* packet)
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
			OnError(dfLOG_LEVEL_SYSTEM, session_id, type, L"RedisArrServer MSG Type Error");
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

void RedisArrServer::OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause)
{
	WCHAR log_buff[df_LOG_BUFF_SIZE];

	if (g_logutils->GetLogLevel() <= log_level)
	{
		wsprintfW(log_buff, L"%s [ErrCode:%d] [SessionID:%I64u]", cause, err_code, session_id);
		g_logutils->Log(NET_LOG_DIR, L"RedisArrServer", log_buff);
	}
}

void RedisArrServer::OnError(const int8 log_level, int32 err_code, const WCHAR* cause)
{
	WCHAR log_buff[df_LOG_BUFF_SIZE];

	if (g_logutils->GetLogLevel() <= log_level)
	{
		wsprintfW(log_buff, L"%s [ErrCode:%d]", cause, err_code);
		g_logutils->Log(NET_LOG_DIR, L"RedisArrServer", log_buff);
	}
}

void RedisArrServer::LogServer(const int8 log_level, const WCHAR* cause)
{

	if (g_logutils->GetLogLevel() <= log_level)
	{
		g_logutils->Log(NET_LOG_DIR, L"RedisArrServer", cause);
	}
}

bool RedisArrServer::OnConnectionRequest(WCHAR* IP, SHORT port)
{
	return true;
}

bool RedisArrServer::OnTimeOut(uint64 session_id)
{
	return true;
}

void RedisArrServer::OnMonitor()
{

}

void RedisArrServer::OnExit()
{

}

void RedisArrServer::HandleRedisSetReq(uint64 session_id, uint16 server_type, int64 key, RedisVal* rd_val)
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
		OnError(dfLOG_LEVEL_SYSTEM, session_id, server_type, L"RedisArrServer Server Type Error");
		Disconnect(session_id);
		return;
	}

	//cout << "Set FIN!!! " << "ServerType: " << server_type << " Key: " << key << " Val: " << rd_val->redis_val << '\n';
	MarshalRedisSetRes(session_id, en_PACKET_CS_REDIS_RES_SET, 1);
}

void RedisArrServer::HandleRedisSetExpireReq(uint64 session_id, uint16 server_type, int64 key, RedisVal* rd_val)
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
		OnError(dfLOG_LEVEL_SYSTEM, session_id, server_type, L"RedisArrServer Server Type Error");
		Disconnect(session_id);
		return;
	}

	//cout << "SetExpire FIN!!! " << "ServerType: " << server_type << " Key: " << key << " Val: " << rd_val->redis_val << " ExpireTime: " << rd_val->expire_time << '\n';
	MarshalRedisSetExpireRes(session_id, en_PACKET_CS_REDIS_RES_SET_EXPIRE, 1);
}

void RedisArrServer::HandleRedisGetReq(uint64 session_id, uint16 server_type, int64 key)
{
	int32 containers_idx = (int32)(key / REDIS_ARR_SIZE);
	int32 arr_idx = key % REDIS_ARR_SIZE;
	RedisArr* redis_arr = &_redis_server_containers[server_type][containers_idx];

	char val[REDIS_VAL_MAX_SIZE];
	uint16 val_len;
	uint8 status;

	{
		SrwLockSharedGuard lock_guard(&redis_arr->_redis_val_arr_srwlock);

		if (redis_arr->_redis_val_arr[arr_idx] != nullptr)
		{
			strcpy_s(val, REDIS_VAL_MAX_SIZE, redis_arr->_redis_val_arr[arr_idx]->redis_val);
			val_len = (uint16)strlen(val);
			status = 1;
		}
		else
		{
			val_len = 0;
			status = 0;
		}

	}

	//cout << "Get FIN!!! " << "ServerType: " << server_type << " Key: " << key << " Val: " << val << '\n';
	MarshalRedisGetRes(session_id, en_PACKET_CS_REDIS_RES_GET, status, val_len, val);
}

void RedisArrServer::HandleRedisDelReq(uint64 session_id, uint16 server_type, int64 key)
{
	int32 containers_idx = (int32)(key / REDIS_ARR_SIZE);
	int32 arr_idx = key % REDIS_ARR_SIZE;
	RedisArr* redis_arr = &_redis_server_containers[server_type][containers_idx];

	{
		SrwLockExclusiveGuard lock_guard(&redis_arr->_redis_val_arr_srwlock);

		if (redis_arr->_redis_val_arr[arr_idx] != nullptr)
		{
			_redis_val_pool->Free(redis_arr->_redis_val_arr[arr_idx]);
			redis_arr->_redis_val_arr[arr_idx] = nullptr;
		}
	}

	//cout << "Del FIN!!! " << "ServerType: " << server_type << " Key: " << key << '\n';
	MarshalRedisDelRes(session_id, en_PACKET_CS_REDIS_RES_SET_EXPIRE, 1);
}


void RedisArrServer::UnmarshalRedisSetReq(uint64 session_id, LanSerializeBuffer* msg)
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

void RedisArrServer::UnmarshalRedisSetExpireReq(uint64 session_id, LanSerializeBuffer* msg)
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

void RedisArrServer::UnmarshalRedisGetReq(uint64 session_id, LanSerializeBuffer* msg)
{
	uint16 server_type;
	int64 key;

	*(msg) >> server_type;
	*(msg) >> key;

	HandleRedisGetReq(session_id, server_type, key);
}

void RedisArrServer::UnmarshalRedisDelReq(uint64 session_id, LanSerializeBuffer* msg)
{
	uint16 server_type;
	int64 key;

	*(msg) >> server_type;
	*(msg) >> key;

	HandleRedisDelReq(session_id, server_type, key);
}

void RedisArrServer::MarshalRedisSetRes(uint64 session_id, uint16 type, uint8 status)
{
	LanSerializeBuffer* msg = ALLOC_LAN_PACKET();
	*(msg) << type << status;
	SendPacket(session_id, msg);
	FREE_LAN_SEND_PACKET(msg);
}

void RedisArrServer::MarshalRedisSetExpireRes(uint64 session_id, uint16 type, uint8 status)
{
	LanSerializeBuffer* msg = ALLOC_LAN_PACKET();
	*(msg) << type << status;
	SendPacket(session_id, msg);
	FREE_LAN_SEND_PACKET(msg);
}

void RedisArrServer::MarshalRedisGetRes(uint64 session_id, uint16 type, uint8 status, uint16 val_len, char* val)
{
	LanSerializeBuffer* msg = ALLOC_LAN_PACKET();
	*(msg) << type << status << val_len;
	msg->PutData(val, val_len);
	SendPacket(session_id, msg);
	FREE_LAN_SEND_PACKET(msg);

}

void RedisArrServer::MarshalRedisDelRes(uint64 session_id, uint16 type, uint8 status)
{
	LanSerializeBuffer* msg = ALLOC_LAN_PACKET();
	*(msg) << type << status;
	SendPacket(session_id, msg);
	FREE_LAN_SEND_PACKET(msg);
}
