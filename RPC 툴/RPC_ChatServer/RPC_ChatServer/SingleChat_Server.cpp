#include "pch.h"
#include "SingleChat_Server.h"
#include "SingleChat_Packet.h"
#include "NetSerializeBuffer.h"
#include "TextParser.h"
#include "LogUtils.h"
#include "PerformanceMonitor.h"
#include "ThreadManager.h"
#include "NetObserver.h"
#include "ChatGroup.h"
#include "MonitorClient.h"

SingleChat_Server* g_chatserver = nullptr;
RedisUtils* g_redis = nullptr;

#pragma warning (disable : 26495)

SingleChat_Server::SingleChat_Server() :_player_num(0), _monitor_client(nullptr), _chat_group(nullptr)
{

}

#pragma warning (default : 26495)


SingleChat_Server::~SingleChat_Server()
{
	delete _monitor_client;
	delete _chat_group;
	delete g_redis;
	delete g_chatserver;
}

void SingleChat_Server::ChatInit(const WCHAR* text_file)
{
	TextParser parser;

	bool textfile_load_ret = parser.LoadFile(text_file);

	if (textfile_load_ret == false)
		__debugbreak();

	WCHAR ip[50];
	WCHAR monitor_ip[50];
	WCHAR log_level[50];
	int32 port;
	int32 monitor_port;
	int32 concurrent_thread_num;
	int32 max_thread_num;
	int32 max_player_num;
	int32 max_session_num;
	int32 accept_job_cnt;
	//int32 backlog_queue_size;
	int32 packet_code;
	int32 packet_key;

	bool nagle_onoff = true;

	parser.GetString(L"BIND_IP", ip);
	parser.GetValue(L"BIND_PORT", &port);
	parser.GetString(L"MONITOR_SERVER_IP", monitor_ip);
	parser.GetValue(L"MONITOR_PORT", &monitor_port);

	parser.GetValue(L"IOCP_WORKER_THREAD", &max_thread_num);
	parser.GetValue(L"IOCP_ACTIVE_THREAD", &concurrent_thread_num);

	parser.GetValue(L"SESSION_MAX", &max_session_num);
	parser.GetValue(L"USER_MAX", &max_player_num);
	parser.GetValue(L"ACCEPT_JOB", &accept_job_cnt);
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

	g_redis = new RedisUtils("127.0.0.1", 6379);
	g_redis->StartRedisThread();


	_monitor_client = new MonitorClient;
	_monitor_client->Init(monitor_ip, monitor_port, 1, 1);

	ChatInit(max_player_num);
	Init(ip, port, concurrent_thread_num, max_thread_num, packet_code, packet_key, true, false, max_session_num, accept_job_cnt, 1000);

	CreateLogicInstanceArr(ContentLogicID::MaxServerID);

	_chat_group = new ChatGroup(this, ContentLogicID::ChatGroupID, 20);
	//부착하기
	AttachLogicInstance(ContentLogicID::ChatGroupID, reinterpret_cast<LogicInstance*>(_chat_group));
	//Timer에 싱글 스레드의 경우 등록
	StartLoopInstance(_chat_group);
}

void SingleChat_Server::ChatInit(int32 max_player_num)
{
	ConsoleSize(43, 100);
	_max_player_num = max_player_num;


	_req_login_tps = 0;
	_req_login_avg = 0;
	_update_tps = 0;
	_update_avg = 0;
	_tcp_retransmission_avg = 0;
}

void SingleChat_Server::ChatStart()
{
	Start();
	_monitor_client->Start();
}

bool SingleChat_Server::EnqueueChatGroupGob(ChatGroupJob chat_job)
{
	return _chat_group->EnqueueJob(chat_job);
}


bool SingleChat_Server::OnAccept(uint64 session_id)
{
	_chat_group->RegisterAccept(session_id);

	if (_player_num >= _max_player_num)
	{
		OnError(dfLOG_LEVEL_SYSTEM, df_CHAT_MAX_PLAYER, L"MAX Player");
		return false;
	}

	return true;
}


bool SingleChat_Server::OnConnectionRequest(WCHAR* IP, SHORT port)
{
	return true;
}

void SingleChat_Server::OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause)
{
	WCHAR log_buff[df_LOG_BUFF_SIZE];

	if (g_logutils->GetLogLevel() <= log_level)
	{
		wsprintfW(log_buff, L"%s [ErrCode:%d] [SessionID:%I64u]", cause, err_code, session_id);
		g_logutils->Log(NET_LOG_DIR, L"ChatLoginServer", log_buff);
	}
}

void SingleChat_Server::OnError(const int8 log_level, int32 err_code, const WCHAR* cause)
{
	WCHAR log_buff[df_LOG_BUFF_SIZE];

	if (g_logutils->GetLogLevel() <= log_level)
	{
		wsprintfW(log_buff, L"%s [ErrCode:%d]", cause, err_code);
		g_logutils->Log(NET_LOG_DIR, L"ChatLoginServer", log_buff);
	}
}

bool SingleChat_Server::OnTimeOut(uint64 session_id)
{
	Disconnect(session_id);
	return true;
}

#pragma warning (disable : 4244)

void SingleChat_Server::OnMonitor()
{
	uint64 console_time = _observer->GetConsoleTime();

	//서버 실행 시간 계산
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


	//PDH 처리
	g_pdh->UpdateAllPDH();

	int32 update_tps = AtomicExchange32ToZero(&_update_tps);

	int32 packet_pool = NetSendPacket::g_send_net_packet_pool.GetChunkUseCount();
	int32 job_queue = _chat_group->UpdateJobQueueSize();
	int32 redis_job_queue = g_redis->GetJobQueueSize();

	int32 player_num = _player_num;
	int32 session_num = _cur_session_num;
	int32 req_login_tps = AtomicExchange32ToZero(&_req_login_tps);

	float process_cpu = g_pdh->ProcessTotal();
	float user_mem = g_pdh->ProcessUserMem_MB();
	float processor_cpu = g_pdh->ProcessorTotal();
	float processor_non_mem = g_pdh->ProcessorNonPageMem_MB();
	int32 prcocssor_recv = g_pdh->ProcessorAdapterRecv_KB();
	int32 prcocssor_send = g_pdh->ProcessorAdapterSend_KB();
	float prcocssor_avail_mem = g_pdh->ProcessorAvailableMem_MB();
	int32 retransmission = g_pdh->ProcessorTCPRetransmission();


	_update_avg += update_tps;
	_req_login_avg += req_login_tps;
	_tcp_retransmission_avg += retransmission;


	uint64 tick = _observer->GetTick();

	PrintLibraryConsole();

	printf("CHATSERVER V1228\n");
	printf("----------------------------------------------------------------------------------------\n");
	printf("Player Num: %d  Req Login TPS: %d\n", player_num, req_login_tps);
	printf("Update       [TPS: %d AVG: %I64u]\n", update_tps, _update_avg / tick);
	printf("Update Job Queue    [Size: %d, Max: %d]\n", job_queue, _chat_group->UpdateJobQueueMaxSize());
	printf("Redis Job Queue     [Size: %d, Max: %d]\n", redis_job_queue, g_redis->GetJobQueueMaxSize());
	printf("Player Node  [Cacpacity: %u Use: %u Release: %u]\n", _chat_group->GetPlayerPoolCapacity(), _chat_group->GetPlayerPoolUseCnt(), _chat_group->GetPlayerPoolReleaseCnt());
	printf("Token Object  [Capacity: %d Alloc: %d Free:%d]\n", _token_pool.GetCapacity(), _token_pool.GetUseCount(), _token_pool.GetReleaseCount());
	printf("RedisChunk     [Cacpacity: %u Use: %u Release: %u]\n", g_redis->GetJobChunkCapacityCount(), g_redis->GetJobChunkUseCount(), g_redis->GetJobChunkReleaseCount());
	printf("RedisChunkNode [Use: %u Release: %u ChunkSize: %d]\n", g_redis->GetJobChunkNodeUseCount(), g_redis->GetJobChunkNodeReleaseCount(), MAX_CHUNK_SIZE);
	printf("----------------------------------------------------------------------------------------\n");
	printf("PDH\n");
	printf("----------------------------------------------------------------------------------------\n");
	printf("CPU HARDWARE [T: %.2lf K: %.2lf U: %.2lf]\n", g_pdh->ProcessorTotal(), g_pdh->ProcessorKernel(), g_pdh->ProcessorUser());
	printf("CPU PROCESS  [T: %.2lf K: %.2lf U: %.2lf]\n", g_pdh->ProcessTotal(), g_pdh->ProcessKernel(), g_pdh->ProcessUser());
	printf("TCP Retransmission [TPS:%d  AVG:%I64u]\n", retransmission, _tcp_retransmission_avg / tick);
	printf("MEM HARDWARE [NonPage:%.2lf  Available:%.2lf]\n", g_pdh->ProcessorNonPageMem_MB(), g_pdh->ProcessorAvailableMem_MB());
	printf("MEM PROCESS  [NonPage:%.2lf  User:%.2lf]\n", g_pdh->ProcessNonPageMem_MB(), g_pdh->ProcessUserMem_MB());
	printf("----------------------------------------------------------------------------------------\n");


	_monitor_client->PrintClientConsole();

	if (_monitor_client->GetConnectFlag())
	{
		printf("Monitor ON\n");
		time_t t;
		int32 cur_time = time(&t);

		ChatGroupJob chat_job;

		chat_job.JobInit(en_JOB_GUI_REQ_SECTOR_INFO);

		_chat_group->EnqueueJob(chat_job);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, 1, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, process_cpu, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, user_mem, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_CHAT_SESSION, session_num, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_CHAT_PLAYER, player_num, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, update_tps, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, packet_pool, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, job_queue, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, processor_cpu, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, processor_non_mem, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, prcocssor_recv, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, prcocssor_send, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, prcocssor_avail_mem, cur_time);

		_monitor_client->SendMontiorPacket(en_PACKET_SS_MONITOR_DATA_UPDATE, dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RETRANSMISSION, retransmission, cur_time);

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


void SingleChat_Server::OnExit()
{
}

