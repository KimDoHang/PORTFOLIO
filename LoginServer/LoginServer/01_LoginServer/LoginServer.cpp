#include "pch.h"
#include "LoginServer.h"
#include "SmartPointer.h"
#include "NetSerializeBuffer.h"
#include "Lock.h"
#include "TextParser.h"
#include "LogUtils.h"
#include "PerformanceMonitor.h"
#include "LoginConfig.h"
#include "LoginValues.h"
#include "AuthServer.h"
#include "NetObserver.h"


#pragma warning(disable : 26495)
LoginServer::LoginServer() : _auth_server(nullptr), _monitor_client(nullptr)
{

}
#pragma warning(default : 26495)


LoginServer::~LoginServer()
{
	delete _auth_server;
	delete _monitor_client;

}



void LoginServer::LoginInit(const WCHAR* text_file)
{
	TextParser parser;

	bool textfile_load_ret = parser.LoadFile(text_file);

	if (textfile_load_ret == false)
		__debugbreak();

	WCHAR ip[AUTH_IP_BUFF_SIZE];
	int32 port;
	WCHAR log_level[50];
	WCHAR monitor_ip[AUTH_IP_BUFF_SIZE];
	WCHAR game_server_ip[AUTH_IP_BUFF_SIZE];
	WCHAR chat_server_ip[AUTH_IP_BUFF_SIZE];
	int32 game_port;
	int32 chat_port;
	int32 monitor_port;
	int32 concurrent_thread_num;
	int32 max_thread_num;
	int32 max_player_num;
	int32 max_session_num;
	int32 packet_code;
	int32 packet_key;
	int32 accept_job;

	bool direct_io = true;
	bool nagle_onoff = true;

	parser.GetString(L"BIND_IP", ip);
	parser.GetValue(L"BIND_PORT", &port);
	parser.GetString(L"GAME_SERVER_BIND_IP", game_server_ip);
	parser.GetValue(L"GAME_SERVER_BIND_PORT", &game_port);
	parser.GetString(L"CHAT_SERVER_BIND_IP", chat_server_ip);
	parser.GetValue(L"CHAT_SERVER_BIND_PORT", &chat_port);

	parser.GetString(L"MONITOR_SERVER_IP", monitor_ip);
	parser.GetValue(L"MONITOR_PORT", &monitor_port);

	parser.GetValue(L"IOCP_WORKER_THREAD", &max_thread_num);
	parser.GetValue(L"IOCP_ACTIVE_THREAD", &concurrent_thread_num);
	parser.GetValue(L"SESSION_MAX", &max_session_num);
	parser.GetValue(L"USER_MAX", &max_player_num);
	parser.GetValue(L"ACCEPT_JOB", &accept_job);


	parser.GetValue(L"PACKET_CODE", &packet_code);
	parser.GetValue(L"PACKET_KEY", &packet_key);
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

	_login_auth_avg = 0;
	_login_auth_tps = 0;
	_update_tps = 0;
	_update_avg = 0;

	ConsoleSize(28, 100);


	_monitor_client = new MonitorClient;
	_monitor_client->Init(monitor_ip, monitor_port, 1, 1, true, false,0,0);


	Init(ip, port, concurrent_thread_num, max_thread_num, packet_code, packet_key, direct_io, nagle_onoff, max_session_num, accept_job, 1000);

	_auth_server = new AuthServer(this, ContentLogicID::AuthServerType, game_server_ip, game_port,chat_server_ip, chat_port);
	CreateLogicInstanceArr(ContentLogicID::MaxServerType);
	AttachLogicInstance(ContentLogicID::AuthServerType, reinterpret_cast<LogicInstance*>(_auth_server));

}

bool LoginServer::OnAccept(uint64 session_id)
{
	_auth_server->RegisterAccept(session_id);

    return true;
}

void LoginServer::OnRelease(uint64 session_id)
{

}



void LoginServer::OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause)
{
	WCHAR log_buff[df_LOG_BUFF_SIZE];

	if (g_logutils->GetLogLevel() <= log_level)
	{
		wsprintfW(log_buff, L"%s [ErrCode:%d] [SessionID:%I64u]", cause, err_code, session_id);
		g_logutils->Log(NET_LOG_DIR, L"LoginServer", log_buff);
	}
}

void LoginServer::OnError(const int8 log_level, int32 err_code, const WCHAR* cause)
{
	WCHAR log_buff[df_LOG_BUFF_SIZE];

	if (g_logutils->GetLogLevel() <= log_level)
	{
		wsprintfW(log_buff, L"%s [ErrCode:%d]", cause, err_code);
		g_logutils->Log(NET_LOG_DIR, L"LoginServer", log_buff);
	}
}
bool LoginServer::OnConnectionRequest(WCHAR* IP, SHORT port)
{
    return true;
}

bool LoginServer::OnTimeOut(uint64 session_id)
{
	Disconnect(session_id);

    return true;
}

#pragma warning (disable : 4244)

void LoginServer::OnMonitor()
{
	uint64 console_time = _observer->GetConsoleTime();

	FILETIME ftCreationTime, ftExitTime, ftKernelTime, ftUsertTime;
	FILETIME ftCurTime;
	GetProcessTimes(GetCurrentProcess(), &ftCreationTime, &ftExitTime, &ftKernelTime, &ftUsertTime);
	GetSystemTimeAsFileTime(&ftCurTime);

	ULARGE_INTEGER start, now;
	start.LowPart = ftCreationTime.dwLowDateTime;
	start.HighPart = ftCreationTime.dwHighDateTime;
	now.LowPart = ftCurTime.dwLowDateTime;
	now.HighPart = ftCurTime.dwHighDateTime;


	ULONGLONG ullElapsedSecond = (now.QuadPart - start.QuadPart) / 10000 / 1000;

	ULONGLONG temp = ullElapsedSecond;

	ULONGLONG ullElapsedMin = ullElapsedSecond / 60;
	ullElapsedSecond %= 60;

	ULONGLONG ullElapsedHour = ullElapsedMin / 60;
	ullElapsedMin %= 60;

	ULONGLONG ullElapsedDay = ullElapsedHour / 24;
	ullElapsedHour %= 24;
	
	g_pdh->UpdateProcessPDH();

	int32 auth_tps =	 AtomicExchange32ToZero(&_login_auth_tps);
	int32 update_tps =	 AtomicExchange32ToZero(&_update_tps);
	int32 packet_pool = NetSendPacket::g_send_net_packet_pool.GetChunkUseCount();

	int32 process_cpu = g_pdh->ProcessTotal();
	int32 process_mem = g_pdh->ProcessUserMem_MB();
	int32 session_num = _cur_session_num;
	int32 chunk_use = NetSendPacket::GetNETChunkUseCount();

	_login_auth_avg += auth_tps;
	_update_avg += update_tps;

	PrintLibraryConsole();

	printf("LOGIN_SERVER V1218 \n");
	printf("----------------------------------------------------------------------------------------\n");
	printf("Update TPS [TPS: %d AVG:%I64u]\n", update_tps, _update_avg / _observer->GetTick());
	printf("Login Auth [TPS: %d AVG:%I64u]\n", auth_tps, _login_auth_avg / _observer->GetTick());


	_monitor_client->PrintClientConsole();

	if (_monitor_client->GetConnectFlag())
	{
		printf("Monitor ON\n");
		time_t tt;;
		int32 cur_time = time(&tt);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN, 1, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU, process_cpu, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM, process_mem, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_LOGIN_SESSION, session_num, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS, auth_tps, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL, chunk_use, cur_time);

		_monitor_client->SendPostOnly();
	}
	else
	{
		printf("Monitor OFF\n");
	}
	
	cout << "LOOP TIME: " << console_time << '\n';
	printf("Elapsed Time : %02lluD-%02lluH-%02lluMin-%02lluSec\n", ullElapsedDay, ullElapsedHour, ullElapsedMin, ullElapsedSecond);
}

#pragma warning (default : 4244)



void LoginServer::OnExit()
{
}


void LoginServer::LoginStart()
{

	_monitor_client->Start();
	Start();
}

void LoginServer::LoginStop()
{
	_monitor_client->Stop();
	Stop();
}




