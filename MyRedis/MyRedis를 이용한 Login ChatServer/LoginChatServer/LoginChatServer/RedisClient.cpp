#include "pch.h"
#include "RedisClient.h"

const WCHAR* REDIS_CLIENT_DIR = L"Redis";
const WCHAR* REDIS_CLIENT_FILE = L"RedisClient";

bool RedisClient::RedisConnect(WCHAR* ip, uint16 port, int32 concurrent_thread_nums, int32 max_thread_nums)
{
	Init(ip, port, concurrent_thread_nums, max_thread_nums);
	Start();
	return true;
}


RedisRet* RedisClient::Set(uint16 server_type, int64 key, const char* val)
{
	ResetEvent(_sync_event);

	LanSerializeBuffer* msg = ALLOC_LAN_PACKET();
	*msg << (uint16)en_PACKET_CS_REDIS_REQ_SET << server_type << key;
	*(msg) << (uint16)strlen(val);
	msg->PutData(val, (int32)strlen(val));
	SendPacket(msg);
	FREE_LAN_SEND_PACKET(msg);

	return &_redis_ret;
}

RedisRet* RedisClient::SetExpire(uint16 server_type, int64 key, int32 expire_time, const char* val)
{
	ResetEvent(_sync_event);

	LanSerializeBuffer* msg = ALLOC_LAN_PACKET();
	*(msg) << (uint16)en_PACKET_CS_REDIS_REQ_SET_EXPIRE << server_type << key << expire_time;
	*(msg) << (uint16)strlen(val);
	msg->PutData(val, (int32)strlen(val));
	SendPacket(msg);
	FREE_LAN_SEND_PACKET(msg);

	return &_redis_ret;
}

RedisRet* RedisClient::Get(uint16 server_type, int64 key)
{
	ResetEvent(_sync_event);

	LanSerializeBuffer* msg = ALLOC_LAN_PACKET();
	*msg << (uint16)en_PACKET_CS_REDIS_REQ_GET << server_type << key;
	SendPacket(msg);
	FREE_LAN_SEND_PACKET(msg);

	return &_redis_ret;
}

RedisRet* RedisClient::Del(uint16 server_type, int64 key)
{
	ResetEvent(_sync_event);

	LanSerializeBuffer* msg = ALLOC_LAN_PACKET();
	*msg << (uint16)en_PACKET_CS_REDIS_REQ_DEL << server_type << key;
	SendPacket(msg);
	FREE_LAN_SEND_PACKET(msg);

	return &_redis_ret;
}


void RedisClient::OnConnect()
{
	g_logutils->Log(REDIS_CLIENT_DIR, REDIS_CLIENT_FILE, L"RedisClient Connect Success");
}

void RedisClient::OnRelease()
{
	g_logutils->Log(REDIS_CLIENT_DIR, REDIS_CLIENT_FILE, L"RedisClient OFF");
}

void RedisClient::OnRecvMsg(LanSerializeBuffer* packet)
{
	uint16 type;
	*packet >> type;

	switch (type)
	{
	case en_PACKET_CS_REDIS_RES_SET:
		UnmarshalRedisSetRes(packet);
		break;
	case en_PACKET_CS_REDIS_RES_SET_EXPIRE:
		UnmarshalRedisSetExpireRes(packet);
		break;
	case en_PACKET_CS_REDIS_RES_GET:
		UnmarshalRedisGetRes(packet);
		break;
	case en_PACKET_CS_REDIS_RES_DEL:
		UnmarshalRedisDelRes(packet);
		break;
	default:
		OnError(dfLOG_LEVEL_SYSTEM, 0, L"RedisServer Client RecvMsg Type Error");
		Disconnect();
		break;
	}

}

void RedisClient::OnError(const int8 log_level, int32 err_code, const WCHAR* cause)
{
	if (g_logutils->GetLogLevel() <= log_level)
	{
		WCHAR log_buff[df_LOG_BUFF_SIZE];
		wsprintfW(log_buff, L"%s [ErrCode:%d]", cause, err_code);

		g_logutils->Log(REDIS_CLIENT_DIR, REDIS_CLIENT_FILE, cause);
	}

}

void RedisClient::OnExit()
{



}

void RedisClient::HandleRedisLoginRes(uint8 status)
{
	_redis_ret._status = status;
	_redis_ret._val[0] = '\0';
	SetEvent(_sync_event);
}

void RedisClient::HandleRedisSetRes(uint8 status)
{
	_redis_ret._status = status;
	_redis_ret._val[0] = '\0';
	SetEvent(_sync_event);
}

void RedisClient::HandleRedisSetExpireRes(uint8 status)
{
	_redis_ret._status = status;
	_redis_ret._val[0] = '\0';
	SetEvent(_sync_event);
}

void RedisClient::HandleRedisGetRes(uint8 status, uint16 val_len, const char* val)
{
	_redis_ret._status = status;
	strncpy_s(_redis_ret._val, val, val_len);
	_redis_ret._val[val_len] = '\0';
	SetEvent(_sync_event);
}

void RedisClient::HandleRedisDelRes(uint8 status)
{
	_redis_ret._status = status;
	_redis_ret._val[0] = '\0';
	SetEvent(_sync_event);
}


void RedisClient::UnmarshalRedisLoginRes(LanSerializeBuffer* msg)
{
	uint8 status;
	*(msg) >> status;

	HandleRedisLoginRes(status);
}

void RedisClient::UnmarshalRedisSetRes(LanSerializeBuffer* msg)
{
	uint8 status;
	*(msg) >> status;

	HandleRedisSetRes(status);
}

void RedisClient::UnmarshalRedisSetExpireRes(LanSerializeBuffer* msg)
{
	uint8 status;
	*(msg) >> status;

	HandleRedisSetExpireRes(status);
}

void RedisClient::UnmarshalRedisGetRes(LanSerializeBuffer* msg)
{
	uint8 status;
	uint16 val_len;

	*(msg) >> status >> val_len;

	HandleRedisGetRes(status, val_len, msg->GetReadPtr());
}

void RedisClient::UnmarshalRedisDelRes(LanSerializeBuffer* msg)
{
	uint8 status;
	*(msg) >> status;

	HandleRedisDelRes(status);
}
