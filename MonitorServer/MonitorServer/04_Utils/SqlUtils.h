#pragma once
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <format>
#include <vector>
#include "mysql.h"
#include "errmsg.h"

#define df_DB_INSERT_ERROR 20001
#define df_DB_UPDATE_ERROR 20002
#define df_DB_SELECT_ERROR 20003
#define df_DB_DELETE_ERROR 20004

#define MAX_QUERY_TIME 1000

struct DBInfo
{
	CHAR ip[20];
	CHAR user[20];
	CHAR pwd[20];
	CHAR db[20];
	int32 port;
};

class SqlUtils
{
public:
	SqlUtils(const char* db_config = "DBServer.cnf");
	~SqlUtils();

	void DBConnect();

	int32 SendQuery(MYSQL* connect, const char* query, int32 query_timeout = MAX_QUERY_TIME);
	int32 SendQueryTLS(const char* query, int32 query_timeout = MAX_QUERY_TIME);

	void OnSQLError(const char* query, MYSQL* connect);
	void OnSQLTimeOut(const char* query, int32 query_time, int32 max_query_time);

	MYSQL_RES* ReadQuery(MYSQL* sql_ptr);
	MYSQL_ROW FetchQuery(MYSQL_RES* sql_result);
	void CloseQuery(MYSQL_RES* sql_result);

	MYSQL* GetConnection() {
		DBConnect();
		return _connect_tls;
	}

private:
	CRITICAL_SECTION _db_cs;
	DBInfo _connect_infos;
	vector<MYSQL*> _connects;

	static thread_local MYSQL* _connect_tls;

};

