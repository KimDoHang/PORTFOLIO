#include "pch.h"
#include "AuthServer.h"



void AuthServer::OnEnter(uint64 session_id, SessionInstance* player)
{
}

void AuthServer::OnLeave(uint64 session_id, SessionInstance* player)
{
}

void AuthServer::OnRelease(uint64 session_id, SessionInstance* player)
{
}

bool AuthServer::OnRecvMsg(uint64 session_id, SessionInstance* player, NetSerializeBuffer* msg)
{
	AtomicIncrement32(&reinterpret_cast<LoginServer*>(_service)->_update_tps);

	WORD type = UINT16_MAX;

	try
	{
		*msg >> type;

		switch (type)
		{
		case en_PACKET_CS_LOGIN_REQ_LOGIN:
			UnmarshalReqLogin(session_id, msg);
			break;
		default:
			_service->OnError(dfLOG_LEVEL_SYSTEM, session_id, 0, L"LoginServer MSG Type Error");
			Disconnect(session_id);
			break;
		}

	}
	catch (const NetMsgException& net_msg_exception)
	{
		_service->OnError(dfLOG_LEVEL_SYSTEM, session_id, type, net_msg_exception.whatW());
		Disconnect(session_id);
	}

	return true;
}

void AuthServer::MarshalResLogin(uint64 session_id, uint16 type, uint64 account_num, uint8 status, WCHAR* id, WCHAR* nick_name, WCHAR* game_ip, USHORT game_port, WCHAR* chat_ip, USHORT chat_port)
{
	NetSerializeBuffer* send_msg = ALLOC_NET_PACKET();

	*send_msg << type << account_num << status;
	send_msg->PutData(reinterpret_cast<char*>(id), MAX_ID_LENGTH);
	send_msg->PutData(reinterpret_cast<char*>(nick_name), MAX_NICKNAME_LENGTH);
	send_msg->PutData(reinterpret_cast<char*>(game_ip), MAX_IP_LENGTH);
	*send_msg << game_port;
	send_msg->PutData(reinterpret_cast<char*>(chat_ip), MAX_IP_LENGTH);
	*send_msg << chat_port;

	_service->SendPacket(session_id, send_msg);
	FREE_NET_SEND_PACKET(send_msg);
}

void AuthServer::MarshalResLoginDisconnect(uint64 session_id, uint16 type, uint64 account_num, uint8 status, WCHAR* id, WCHAR* nick_name, WCHAR* game_ip, USHORT game_port, WCHAR* chat_ip, USHORT chat_port)
{
	NetSerializeBuffer* send_msg = ALLOC_NET_PACKET();

	*send_msg << type << account_num << status;
	send_msg->PutData(reinterpret_cast<char*>(id), MAX_ID_LENGTH);
	send_msg->PutData(reinterpret_cast<char*>(nick_name), MAX_NICKNAME_LENGTH);
	send_msg->PutData(reinterpret_cast<char*>(game_ip), MAX_IP_LENGTH);
	*send_msg << game_port;
	send_msg->PutData(reinterpret_cast<char*>(chat_ip), MAX_IP_LENGTH);
	*send_msg << chat_port;

	_service->SendDisconnect(session_id, send_msg);
	FREE_NET_SEND_PACKET(send_msg);
}

void AuthServer::HandleReqLogin(uint64 session_id, int64 account_num, char* session_key)
{
	WCHAR id[df_ID_BUFF_SIZE];
	WCHAR nick_name[df_NICK_BUFF_SIZE];
	BYTE ret;

	do
	{
		ret = CheckPlatform(account_num, session_key, id, nick_name);

		if (ret != 1)
		{
			_service->OnError(dfLOG_LEVEL_SYSTEM, ret, L"CheckPlatForm Fail");
			break;
		}

		ret = TokenWrite(account_num, session_key);

		if (ret != 1)
		{
			_service->OnError(dfLOG_LEVEL_SYSTEM, ret, L"Reddiss Write Fail");
			break;
		}

		MarshalResLogin(session_id, en_PACKET_CS_LOGIN_RES_LOGIN, account_num, ret, id, nick_name, _game_server_ip, _game_server_port, _chat_server_ip, _chat_server_port);
		AtomicIncrement32(&reinterpret_cast<LoginServer*>(_service)->_login_auth_tps);
		return;

	} while (false);

	//인증 실패시 관련 에러 코드와 함께 SendDisconnect 방식으로 에러코드를 전달
	MarshalResLoginDisconnect(session_id, en_PACKET_CS_LOGIN_RES_LOGIN, account_num, ret, id, nick_name, _game_server_ip, _game_server_port, _chat_server_ip, _chat_server_port);

	return;
}

BYTE AuthServer::CheckPlatform(uint64 account_num, char* session_key, WCHAR* id, WCHAR* nick_name)
{
	int32 err_code;
	MYSQL_RES* sql_result;
	MYSQL_ROW sql_row;
	size_t size;
	char query[df_QUERY_BUFFER_SIZE];

	MYSQL* connect = _db_manager->GetConnection();

	sprintf_s(query, df_QUERY_BUFFER_SIZE, "SELECT `sessionkey` FROM `accountdb`.`sessionkey` WHERE accountno = %I64u;", account_num);
	_db_manager->SendQuery(connect, query);
	sql_result = _db_manager->ReadQuery(connect);


	_db_manager->CloseQuery(sql_result);


	sprintf_s(query, df_QUERY_BUFFER_SIZE, "SELECT `userid`, `usernick` FROM `accountdb`.`account` WHERE accountno = %I64u;", account_num);
	_db_manager->SendQuery(connect, query);
	sql_result = _db_manager->ReadQuery(connect);


	sql_row = _db_manager->FetchQuery(sql_result);

	if (sql_row == NULL)
	{
		_db_manager->CloseQuery(sql_result);
		return 2;
	}

	err_code = mbstowcs_s(&size, id, df_ID_BUFF_SIZE, sql_row[0], SIZE_MAX);

	if (err_code != 0)
	{
		_service->OnError(dfLOG_LEVEL_SYSTEM, static_cast<int32>(size), L"mbstowcs_s Fail");
		__debugbreak();
	}

	err_code = mbstowcs_s(&size, nick_name, df_NICK_BUFF_SIZE, sql_row[1], SIZE_MAX);

	if (err_code != 0)
	{
		_service->OnError(dfLOG_LEVEL_SYSTEM, static_cast<int32>(size), L"mbstowcs_s Fail");
		__debugbreak();
	}

	_db_manager->CloseQuery(sql_result);

	return 1;
}

int32 AuthServer::TokenWrite(uint64 account_num, char* session_key)
{
	char key[30];

	sprintf_s(key, "ChatServer:%I64u", account_num);
	cpp_redis::reply&& reply = _redis_manger->SetSync(key, session_key, "60");

	if (reply.is_error() == true)
	{
		char c_log_buff[df_LOG_BUFF_SIZE];
		WCHAR* log_ret = g_logutils->GetLogBuffPtr();
		size_t size;
		sprintf_s(c_log_buff, df_LOG_BUFF_SIZE, "RedisError [Error:%s]", reply.error().c_str());
		mbstowcs_s(&size, log_ret, df_LOG_BUFF_SIZE, c_log_buff, SIZE_MAX);
		g_logutils->Log(NET_LOG_DIR, L"Redis", log_ret);
		return 5;
	}

	return 1;
}