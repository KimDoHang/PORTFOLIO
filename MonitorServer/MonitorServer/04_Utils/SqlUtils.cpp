#include "pch.h"
#include "SqlUtils.h"
#include "TextParser.h"
#include "LogUtils.h"

thread_local MYSQL* SqlUtils::_connect_tls = nullptr;

SqlUtils::SqlUtils(const char* db_config)
{
	InitializeCriticalSection(&_db_cs);

	TextParser parser;

	bool textfile_load_ret = parser.LoadFile(db_config);

	if (textfile_load_ret == false)
		__debugbreak();

	parser.GetString("BIND_IP", _connect_infos.ip);
	parser.GetString("DB_ID", _connect_infos.user);
	parser.GetString("DB_PSW", _connect_infos.pwd);
	parser.GetString("DB_NAME", _connect_infos.db);
	parser.GetValue("BIND_PORT", &_connect_infos.port);

}

SqlUtils::~SqlUtils()
{

	EnterCriticalSection(&_db_cs);

	for (auto iter = _connects.begin(); iter != _connects.end(); iter++)
	{
		mysql_close(*iter);
	}

	LeaveCriticalSection(&_db_cs);

	DeleteCriticalSection(&_db_cs);
}

void SqlUtils::DBConnect()
{
	if (_connect_tls != nullptr)
		return;


	EnterCriticalSection(&_db_cs);

	_connect_tls = new MYSQL;
	_connects.push_back(_connect_tls);

	mysql_init(_connect_tls);
	mysql_real_connect(_connect_tls, _connect_infos.ip, _connect_infos.user, _connect_infos.pwd, _connect_infos.db, _connect_infos.port, (char*)NULL, 0);

	if (_connect_tls == NULL)
	{
		OnSQLError("MySQL Connection Error", _connect_tls);
		__debugbreak();
	}

	bool reconnect = true;
	mysql_options(_connect_tls, MYSQL_OPT_RECONNECT, &reconnect);


	LeaveCriticalSection(&_db_cs);
}

int32 SqlUtils::SendQuery(MYSQL* connect, const char* query, int32 query_timeout)
{

	int32 query_time = timeGetTime();
	int32 query_ret = mysql_query(connect, query);
	query_time = timeGetTime() - query_time;

	if (query_ret != 0)
	{
		query_ret = mysql_errno(_connect_tls);
		OnSQLError(query, _connect_tls);
		return query_ret;
	}

	if (query_time > query_time)
		OnSQLTimeOut(query, query_time, query_timeout);

	return query_ret;
}

int32 SqlUtils::SendQueryTLS(const char* query, int32 query_timeout)
{
	DBConnect();

	int32 query_time = timeGetTime();
	int32 query_ret = mysql_query(_connect_tls, query);
	query_time = timeGetTime() - query_time;

	if (query_ret != 0)
	{
		query_ret = mysql_errno(_connect_tls);
		OnSQLError(query, _connect_tls);
		return query_ret;
	}

	if (query_time > query_time)
		OnSQLTimeOut(query, query_time, query_timeout);

	return query_ret;
}

void SqlUtils::OnSQLError(const char* query, MYSQL* connect)
{
	char c_log_buff[df_LOG_BUFF_SIZE];
	WCHAR* log_ret = g_logutils->GetLogBuffPtr();
	size_t size;

	sprintf_s(c_log_buff, df_LOG_BUFF_SIZE, "%s [SQL Error: %s]\n", query, mysql_error(connect));
	mbstowcs_s(&size, log_ret, df_LOG_BUFF_SIZE, c_log_buff, SIZE_MAX);
	g_logutils->Log(NET_LOG_DIR, L"SQL_DB", log_ret);
}

void SqlUtils::OnSQLTimeOut(const char* query, int32 query_time, int32 max_query_time)
{
	char c_log_buff[df_LOG_BUFF_SIZE];
	WCHAR* log_ret = g_logutils->GetLogBuffPtr();
	size_t size;

	sprintf_s(c_log_buff, df_LOG_BUFF_SIZE, "Query Time Out [Query:%s] [query_time:%d, max_query_time:%d]", query, query_time, max_query_time);
	mbstowcs_s(&size, log_ret, df_LOG_BUFF_SIZE, c_log_buff, SIZE_MAX);
	g_logutils->Log(NET_LOG_DIR, L"SQL_DB", log_ret);
}

MYSQL_RES* SqlUtils::ReadQuery(MYSQL* sql_ptr)
{
	return mysql_store_result(sql_ptr);
}
MYSQL_ROW SqlUtils::FetchQuery(MYSQL_RES* sql_result)
{
	return mysql_fetch_row(sql_result);
}

void SqlUtils::CloseQuery(MYSQL_RES* sql_result)
{
	mysql_free_result(sql_result);
}

