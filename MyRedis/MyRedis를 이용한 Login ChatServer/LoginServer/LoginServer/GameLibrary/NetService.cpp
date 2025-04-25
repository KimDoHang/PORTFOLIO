#include "pch.h"
#include "NetService.h"
#include "SocketUtils.h"
#include "NetSession.h"
#include "LogUtils.h"
#include "Lock.h"
#include "LockFreeQueue.h"
#include "TextParser.h"
#include "NetRecvPacket.h"
#include "NetSendPacket.h"
#include "LockFreeQueueStatic.h"
#include "ThreadManager.h"
#include "LogicInstance.h"
#include "GroupInstance.h"
#include "LoopInstance.h"
#include "NetObserver.h"

#pragma warning (disable : 4700)
#pragma warning (disable : 4703)


#pragma warning (disable : 26495)

NetService::NetService() : _session_cnt(0), _type(ServiceType::NetServerType), _logic_content_arr(nullptr), _logic_content_arr_size(0), _sessions(nullptr), _kicker(nullptr), _observer(nullptr), _session_pool(nullptr), _logic_instance_num(0)
{
	_thread_manager = new ThreadManager;
}
#pragma warning (default : 26495)


NetService::~NetService()
{
	delete[] _sessions;
	delete _logic_content_arr;
	delete _kicker;
	delete _observer;
	delete _session_pool;
	delete _thread_manager;
}

bool NetService::Init(const WCHAR* ip, SHORT port, int32 concurrent_thread_num, int32 max_thread_num, uint8 packet_code, uint8 packet_key, bool direct_io,  bool nagle_on, int32 max_session_num, int32 accept_job_cnt, int32 monitor_time, int32 timeout_time, int32 backlog_size)
{
	int32 err_code = 0;
	WCHAR* log_buff;
	size_t size;

	_session_pool = new LockFreeObjectPool<NetSession, false>;
	//출력을 위한 값 설정
	_port = port;
	wcstombs_s(&size, _ip, IPBUF_SIZE, ip, SIZE_MAX);

	//서버 타입이 및 시작 처리 -> 나중에 손보자
	_exit_flag = 0;

	//패킷 코드 등록
	_packet_code = packet_code;
	_packet_key = packet_key;

	//Session 초기화 (MAX 및 할당 MAX 수치)
	_max_session_num = max_session_num;
	_cur_session_num = 0;
	_alloc_session_num = 0;

	//Content 관련 변수들 초기화
	_logic_content_arr = nullptr;
	_logic_content_arr_size = 0;

	NetAddress::SetAddress(_server_addr, ip, port);

	_sessions = new NetSession[_max_session_num];

	//IO Start
	CreateIOThread(concurrent_thread_num, max_thread_num);

	log_buff = g_logutils->GetLogBuff(L"------ Net Server 시작 [IP: %s, Port: %hd] [MaxSession: %d] ------", ip, port, max_thread_num);
	g_logutils->Log(NET_LOG_DIR, NET_FILE_NAME, log_buff);

	log_buff = g_logutils->GetLogBuff(L"IO Thread 시작! [ConcurrentThread : %d, MaxThreadNum : %d]", concurrent_thread_num, max_thread_num);
	g_logutils->Log(NET_LOG_DIR, NET_FILE_NAME, log_buff);

	//IOCP가 IO Thread 생성시에 생성된다. 
	for (int i = _max_session_num - 1; i >= 0; i--)
	{
		_session_idx_stack.push(i);
		_sessions[i]._type = ServiceType::NetServerType;
	}

	//Frame Thread 생성
	CreateFrameTimerThread();

	//Monitor Start
	CreateObserverThread(monitor_time);
	//TimeOut Start
	CreateTimeOutThread(timeout_time);

	AcceptStart(direct_io, nagle_on, backlog_size, accept_job_cnt);

	return true;
}

void NetService::Start()
{
	_thread_manager->Start();
}

void NetService::Stop()
{
	InterlockedExchange8(&_exit_flag, 1);

	closesocket(_listen_socket);

	while (_alloc_session_num != 0)
	{
		for (int i = 0; i < _max_session_num; i++)
		{
			Disconnect(_sessions[i]._session_id);
		}
		Sleep(100);
	}

	for (int idx = 0; idx < _logic_content_arr_size; idx++)
	{
		if (_logic_content_arr[idx] == nullptr)
			continue;
		_logic_content_arr[idx]->RegisterExit();
	}

	while (_logic_instance_num != 0)
	{

	}

	for (int cnt = 0; cnt < _iocp_core._thread_nums; cnt++)
	{
		PostQueuedCompletionStatus(_iocp_core._iocp_handle, 0, 0, NULL);
	}
	
	//Frame Timer 종료
	Job* job = JOB_ALLOC();
	job->type = JobType::ExitJob;
	_frame_timer._job_queue.Enqueue(job);
	if (_frame_timer._job_queue.Enqueue(job) == 1)
	{
		WakeByAddressSingle(_frame_timer._job_queue.SizePtr());
	}

	OnExit();

	_thread_manager->Join();
}

void NetService::AcceptStart(bool direct_io, bool nagle_on, int32 backlog_size, int32 accept_job_cnt)
{
	int32 err_code = 0;

	_listen_socket = SocketUtils::CreateSocket();

	WCHAR log_buff[df_LOG_BUFF_SIZE];

	if (_listen_socket == INVALID_SOCKET)
	{
		API_SYSTEM_BREAKLOG(dfLOG_LEVEL_SYSTEM, err_code, L"CreateSocket Fail");
	}
	API_SYSTEM_LOG(dfLOG_LEVEL_SYSTEM, err_code, L"CreateSocket Success");

	if (_iocp_core.Register(_listen_socket) == false)
	{
		API_SYSTEM_BREAKLOG(dfLOG_LEVEL_SYSTEM, err_code, L"ListenSocket IOCP Core Register Fail [ErrCode:%d]");
	}
	API_SYSTEM_LOG(dfLOG_LEVEL_SYSTEM, err_code, L"ListenSocket IOCP Core Register Success");

	if (SocketUtils::SetLinger(_listen_socket, true, 0) == false)
	{
		API_SYSTEM_BREAKLOG(dfLOG_LEVEL_SYSTEM, err_code, L"SetLinger Fail [ErrCode:%d]");
	}
	API_SYSTEM_LOG(dfLOG_LEVEL_SYSTEM, err_code, L"SetLinger Success");


	if (SocketUtils::SetSendBufferSize(_listen_socket, 0) == false)
	{
		API_SYSTEM_BREAKLOG(dfLOG_LEVEL_SYSTEM, err_code, L"SetSendBufferSize Fail [ErrCode:%d]");
	}
	API_SYSTEM_LOG(dfLOG_LEVEL_SYSTEM, err_code, L"DirectIO Success");



	if (SocketUtils::SetSendBufferSize(_listen_socket, 0) == false)
	{
		API_SYSTEM_BREAKLOG(dfLOG_LEVEL_SYSTEM, err_code, L"SetSendBufferSize Fail [ErrCode:%d]");
	}
	API_SYSTEM_LOG(dfLOG_LEVEL_SYSTEM, err_code, L"DirectIO Success");

	/*if (direct_io)
	{
		if (SocketUtils::SetSendBufferSize(_listen_socket, 0) == false)
		{
			API_SYSTEM_BREAKLOG(dfLOG_LEVEL_SYSTEM, err_code, L"SetSendBufferSize Fail [ErrCode:%d]");
		}
		API_SYSTEM_LOG(dfLOG_LEVEL_SYSTEM, err_code, L"DirectIO Success");
	}
	else
	{
		API_SYSTEM_LOG(dfLOG_LEVEL_SYSTEM, err_code, L"BufferedIO Success");
	}



	if (nagle_on == false)
	{
		if (SocketUtils::SetNagle(_listen_socket, true) == false)
		{
			API_SYSTEM_BREAKLOG(dfLOG_LEVEL_SYSTEM, err_code, L"SetNagle OFF Fail [ErrCode:%d]");
		}
		API_SYSTEM_LOG(dfLOG_LEVEL_SYSTEM, err_code, L"SetNagle OFF Success");
	}
	else
	{
		API_SYSTEM_LOG(dfLOG_LEVEL_SYSTEM, err_code, L"SetNagle ON Success");
	}*/

	if (SocketUtils::Bind(_listen_socket, _server_addr) == false)
	{
		API_SYSTEM_BREAKLOG(dfLOG_LEVEL_SYSTEM, err_code, L"Bind Fail [ErrCode:%d]");
	}
	API_SYSTEM_LOG(dfLOG_LEVEL_SYSTEM, err_code, L"Bind Success");

	if (SocketUtils::Listen(_listen_socket, SOMAXCONN_HINT(backlog_size)) == false)
	{
		API_SYSTEM_BREAKLOG(dfLOG_LEVEL_SYSTEM, err_code, L"Listen Fail [ErrCode:%d]");
	}

	wsprintfW(log_buff, L"Listen Success [Accept Job:%d]", accept_job_cnt);
	API_SYSTEM_LOG(dfLOG_LEVEL_SYSTEM, err_code, log_buff);

	for (int32 i = 0; i < accept_job_cnt; i++)
	{
		AcceptPost();
	}
}


void NetService::CreateIOThread(int concurrent_thread_nums, int max_thread_nums)
{
	_iocp_core.IOInit(this, concurrent_thread_nums, max_thread_nums);

	for (int i = 0; i < max_thread_nums; i++)
	{
		_thread_manager->Launch(IocpCore::NetIOThread, this, ServiceType::NetServerType, MonitorInfoType::IOInfoType);
	}
}

void NetService::CreateObserverThread(int32 observer_time)
{
	_observer = new NetObserver(this, observer_time);
	if (observer_time != 0)
	{
		StartLoopInstance(_observer);
		WCHAR* log_buff = g_logutils->GetLogBuff(L"Monitor Thread 시작! [MonitorTime : %d]", observer_time);
		g_logutils->Log(NET_LOG_DIR, NET_FILE_NAME, log_buff);
	}
}

void NetService::CreateTimeOutThread(int32 timeout_time)
{
	_kicker = new Kicker(this, timeout_time);

	if (timeout_time != 0)
	{
		StartLoopInstance(_kicker);
		WCHAR* log_buff = g_logutils->GetLogBuff(L"TimeOut Thread 시작! [TimeOutTime : %d]", timeout_time);
		g_logutils->Log(NET_LOG_DIR, NET_FILE_NAME, log_buff);
	}
}

void NetService::CreateFrameTimerThread()
{
	_thread_manager->Launch(FrameTimer::FrameTimerThread, this, ServiceType::NetServerType, MonitorInfoType::NoneInfo);
}

void NetService::IOThreadLoop()
{
	int32 err_code = 0;
	DWORD gqcs_transferred_dw;
	NetSession* session;
	ULONG_PTR key;
	OverlappedEvent* overlapped;
	LoopInstance* loop_instance;

	_thread_manager->Wait();


	while (true)
	{
		gqcs_transferred_dw = 0;
		key = 0;
		overlapped = nullptr;

		bool iocp_ret = GetQueuedCompletionStatus(_iocp_core._iocp_handle, &gqcs_transferred_dw, &key, reinterpret_cast<LPOVERLAPPED*>(&overlapped), INFINITE);

		if (overlapped == nullptr && gqcs_transferred_dw == 0 && key == 0)
		{
				if (iocp_ret == true)
				{
					LIB_SYSTEM_LOG(dfLOG_LEVEL_SYSTEM, 0, L"IOCP Thread 종료");
					break;
				}
				else
				{
					err_code = WSAGetLastError();
					LIB_SYSTEM_BREAKLOG(dfLOG_LEVEL_SYSTEM, err_code, L"IOCP Thread Fail");
					return;
				}
		}
		

		switch (overlapped->type)
		{
		case OverlappedEventType::ACCEPT_OVERLAPPED:
			session = reinterpret_cast<NetSession*>(overlapped->instance);
			if (!iocp_ret || AcceptProc(session) == false)
			{
				ResetProc(session);
				AcceptPost();
				continue;
			}
			break;
		case OverlappedEventType::MAXACCEPT_OVERLAPPED:
			session = reinterpret_cast<NetSession*>(overlapped->instance);
			MaxAcceptProc(session);
			continue;
		case OverlappedEventType::RELEASE_OVERLAPPED:
			session = reinterpret_cast<NetSession*>(overlapped->instance);
			ReleaseProc(session);
			continue;
		case OverlappedEventType::SEND_OVERLAPPED:
			session = reinterpret_cast<NetSession*>(overlapped->instance);
			if (iocp_ret)
			{
				SendProc(session);
			}
			break;
		case OverlappedEventType::RECV_OVERLAPPED:
			session = reinterpret_cast<NetSession*>(overlapped->instance);
			if (iocp_ret && gqcs_transferred_dw != 0)
			{
				RecvProc(session, gqcs_transferred_dw);
			}
			break;
		case OverlappedEventType::SESSION_POST_OVERLAPPED:
			session = static_cast<NetSession*>(overlapped->instance);
			SendPost(session);
			break;
		case OverlappedEventType::LOOP_OVERLAPPED:
			loop_instance = static_cast<LoopInstance*>(overlapped->instance);
			loop_instance->Loop();
			continue;
		default:
			LIB_SYSTEM_BREAKLOG(dfLOG_LEVEL_SYSTEM, err_code, L"OVERLAPPED TYPE ERROR");
			break;
		}

		if (AtomicDecrement32(&session->_io_cnt) == 0)
		{
			session->_logic_instance->RegisterRelease(session);
		}
	}


	return;
}


bool NetService::TimeOutThreadLoop()
{
	uint64 session_id;
	uint64 cur_time;

	if (_exit_flag)
		return false;

	cur_time = GetTickCount64();

	for (int i = 0; i < _max_session_num; i++)
	{
		//한번도 할당되지 않았거나 이미 삭제된 session이면 무시한다.
		if ((_sessions[i]._io_cnt & NetSession::RELEASE_FLAG_ON_MASK) != 0)
			continue;
		
		//미리 sessionID를 저장하여 과거 혹은 현재의 id를 가져온다.
		session_id = _sessions[i]._session_id;

		//session_ID를 할당한 이후 session의 time을 초기화 하기 떄문에
		//session의 time은 반드시 ID가 재할당 된 이후이거나 과거의 ID이다.
		if (_sessions[i]._time == 0)
			continue;

		//timeout time을 frame_tick마다 돈다.
		if (_sessions[i]._time + _kicker->_frame_tick < cur_time)
		{
			//Disconnect 호출 시 현재 session의 session_id가 일치하는지 여부를 다시 한번 확인한다.
			OnTimeOut(session_id);
			WCHAR* log_buff = g_logutils->GetLogBuff(L"TimeOut Session [SessionID : %I64u]", session_id);
			g_logutils->Log(NET_LOG_DIR, NET_FILE_NAME, log_buff);
		}
	}
	

	return true;
}

void NetService::PrintLibraryConsole()
{

	printf("========================================================================================\n");
	printf("NET SERVER LiBRARY V1218-2 [IP: %s PORT: %hu MAX_THREAD_NUM: %d CONCURENT_THREAD: %d]\n", _ip, _port, _iocp_core._thread_nums, _iocp_core._concurrent_thread_nums);
	printf("----------------------------------------------------------------------------------------\n");
	printf("Session Num : %d  MAX Session : %d  Alloc Session : %I64u\n", _cur_session_num, _max_session_num, _alloc_session_num);
	printf("Accept        [TPS: %d AVG: %I64u TOTAL: %I64u] \n", _observer->_accept_tps, (_observer->_accept_total / _observer->_out_tick), _observer->_accept_total);
	printf("SendMSG       [TPS: %d AVG: %I64u]\n", _observer->_out_send_msg_tps, _observer->_send_msg_avg / _observer->_out_tick);
	printf("RecvMSG       [TPS: %d AVG: %I64u]\n", _observer->_out_recv_msg_tps, _observer->_recv_msg_avg / _observer->_out_tick);
	printf("SendChunk     [Cacpacity: %u Use: %u Release: %u]\n", NetSendPacket::g_send_net_packet_pool.GetChunkCapacityCount(), NetSendPacket::g_send_net_packet_pool.GetChunkUseCount(), NetSendPacket::g_send_net_packet_pool.GetChunkReleaseCount());
	printf("SendChunkNode [Use: %u Release: %u ChunkSize: %d]\n", NetSendPacket::g_send_net_packet_pool.GetChunkNodeUseCount(), NetSendPacket::g_send_net_packet_pool.GetChunkNodeReleaseCount(), MAX_CHUNK_SIZE);
	printf("RecvChunk     [Cacpacity: %u Use: %u Release: %u]\n", NetRecvPacket::g_recv_net_packet_pool.GetChunkCapacityCount(), NetRecvPacket::g_recv_net_packet_pool.GetChunkUseCount(), NetRecvPacket::g_recv_net_packet_pool.GetChunkReleaseCount());
	printf("RecvChunkNode [Use: %u Release: %u ChunkSize: %d]\n", NetRecvPacket::g_recv_net_packet_pool.GetChunkNodeUseCount(), NetRecvPacket::g_recv_net_packet_pool.GetChunkNodeReleaseCount(), MAX_CHUNK_SIZE);
	printf("Timer Queue   [Size: %d, Max: %d]\n", _frame_timer._job_queue.Size(), df_QUEUE_SIZE);
	printf("JobChunk      [Cacpacity: %u Use: %u Release: %u]\n", g_job_pool.GetChunkCapacityCount(), g_job_pool.GetChunkUseCount(), g_job_pool.GetChunkReleaseCount());
	printf("JobChunkNode  [Use: %u Release: %u ChunkSize: %d]\n", g_job_pool.GetChunkNodeUseCount(), g_job_pool.GetChunkNodeReleaseCount(), MAX_CHUNK_SIZE);
	printf("----------------------------------------------------------------------------------------\n");
}

bool NetService::ObserverThreadLoopIOCP()
{
	if (_exit_flag)
		return false;

	OnMonitor();
	return true;
}

void NetService::FrameTimerThreadLoop()
{
	long size;
	Job* job;
	uint64 next_tick;
	uint64 cur_tick;
	LoopInstance* loop_instance;

	_thread_manager->Wait();

	while (true)
	{
		while (true)
		{
			size = _frame_timer._job_queue.Size();

			if (size == 0)
			{
				//먼저 Timer의 Job을 모두 처리한 후 Post해야하는 Frame 확인
				next_tick = PostFrame();
				break;
			}

			_frame_timer._job_queue.Dequeue(job);

			switch (job->type)
			{
			case JobType::RegisterJob:
			{
				cur_tick = timeGetTime();
				loop_instance = static_cast<LoopInstance*>(job->instance);
				//Group의 Frame 및 Timer의 Tick coefficient 계수에 따라 다음 post 시간 결정
				loop_instance->_last_tick += loop_instance->_frame_tick * _frame_timer._tick_coefficient;

				//이미 Frame을 충족하면 Timer에 등록하지 않고 바로 Post
				if (cur_tick > loop_instance->_last_tick)
				{
					PostQueuedCompletionStatus(_iocp_core._iocp_handle, 0, 0, &loop_instance->_loop_overlapped);
					break;
				}
				//Frame이 안되었다면 Timer에 등록
				_frame_timer._timeinfo_pq.push(TimeInfo(loop_instance, loop_instance->_last_tick));
				break;
			}
			case JobType::RemoveJob:
				break;
			case JobType::ExitJob:
				JOB_FREE(job);
				return;
			default:
			{
				WCHAR* lob_buff = g_logutils->GetLogBuff(L"JobType Not Defined");
				OnError(dfLOG_LEVEL_SYSTEM, job->session_id, df_JOBTYPE_NOT_DEFINED_ERROR, lob_buff);
				break;
			}
			}

			JOB_FREE(job);
		}

		cur_tick = timeGetTime();

		if (cur_tick > next_tick)
		{
			next_tick = 0;
		}
		else
			next_tick -= cur_tick;

#pragma warning (disable : 4244)
		WaitOnAddress(_frame_timer._job_queue.SizePtr(), &size, sizeof(long), next_tick);
#pragma warning (default : 26495)

	}
}

uint32 NetService::PostFrame()
{
	uint32 cur_tick;

	while (true)
	{
		if (_frame_timer._timeinfo_pq.size() == 0)
		{
			cur_tick = INFINITE;
			break;
		}

		TimeInfo info = _frame_timer._timeinfo_pq.top();

		cur_tick = timeGetTime();

		if (info.next_frame_tick > cur_tick)
		{
#pragma warning (disable : 4244)

			cur_tick = info.next_frame_tick;
#pragma warning (default : 4244)

			break;
		}

		_frame_timer._timeinfo_pq.pop();
		PostQueuedCompletionStatus(_iocp_core._iocp_handle, 0, 0, reinterpret_cast<OVERLAPPED*>(&info.instance->_loop_overlapped));
	}

	return cur_tick;
}

void NetService::CreateLogicInstanceArr(uint16 max_content_size)
{
	if (_logic_content_arr != nullptr)
	{
		OnError(dfLOG_LEVEL_SYSTEM, 0, L"Already Group Arr Exist");
		__debugbreak();
	}

	_logic_content_arr = new LogicInstance*[max_content_size];
	_logic_content_arr_size = max_content_size;

	for (int i = 0; i < _logic_content_arr_size; i++)
	{
		_logic_content_arr[i] = nullptr;
	}
}

LogicInstance* NetService::GetLogicInstance(uint16 logic_id)
{
	if (logic_id >= _logic_content_arr_size)
	{
		OnError(dfLOG_LEVEL_SYSTEM, 0, L"Group Arr Over Exceed");
		__debugbreak();
	}

	LogicInstance* logic_instance = _logic_content_arr[logic_id];

	if (logic_instance == nullptr)
	{
		OnError(dfLOG_LEVEL_SYSTEM, 0, L"Group Not Attach");
		__debugbreak();
	}

	return logic_instance;
}

void NetService::AttachLogicInstance(uint16 logic_id, LogicInstance* logic_instance)
{
	if (_logic_content_arr == nullptr)
	{
		OnError(dfLOG_LEVEL_SYSTEM, 0, L"Group Arr Not Exist");
		__debugbreak();
	}

	if (_logic_content_arr[logic_id] != nullptr)
	{
		OnError(dfLOG_LEVEL_SYSTEM, 0, L"Group Already Attach");
		__debugbreak();
	}

	_logic_content_arr[logic_id] = logic_instance;


	return;
}

bool NetService::DetachGroupInstance(uint16 logic_id)
{
	return false;
}

void NetService::StartLoopInstance(LoopInstance* loop_instance)
{
	AtomicIncrement16(&_logic_instance_num);
	loop_instance->SetLastTick();
	PostQueuedCompletionStatus(_iocp_core._iocp_handle, 0, 0, reinterpret_cast<OVERLAPPED*>(&loop_instance->_loop_overlapped));
}

void NetService::ExitLoopInstance()
{
	AtomicDecrement16(&_logic_instance_num);
}

void NetService::RegisterLoop(LoopInstance* loop_instance)
{
	loop_instance->_past_console_time = loop_instance->_cur_console_time;
	Job* job = JOB_ALLOC();
	job->JobInit(JobType::RegisterJob, loop_instance);
	if (_frame_timer._job_queue.Enqueue(job) == 1)
	{
		WakeByAddressSingle(_frame_timer._job_queue.SizePtr());
	}
}



//외부에서 호출하는 함수이기 때문에 해당 하는 Session이 지워졌을 수 있다.
//따라서 GetSession을 통해서 SendPacket과 마찬가지로 확인 및 삭제방지를 처리해야 한다.
bool NetService::RegisterSessionInstance(uint64 session_id, SessionInstance* instance)
{
	//session io_cnt 증가 -> 삭제 방지
	NetSession* session = GetSession(session_id);

	if (session == nullptr)
		return false;

	session->_instance = instance;

	//io_cnt 감소
	ReturnSession(session);

	return true;
}

bool NetService::SendPacket(uint64 session_id, NetSerializeBuffer* packet)
{
	NetSession* session = GetSession(session_id);

	if (session == nullptr)
	{
		return false;
	}

	//Header Setting
	SetPacket(packet);
	packet->AddCnt();
	if (session->_send_buf.Enqueue(packet) == df_QUEUE_SIZE)
	{
		OnError(dfLOG_LEVEL_SYSTEM, session_id, df_RECVQ_FULL, L"Session Queue Full");
		DisconectInLib(session);
	}

	SendPost(session);

	if (AtomicDecrement32(&session->_io_cnt) == 0)
	{
		session->_logic_instance->RegisterRelease(session);
	}

	return true;
}

bool NetService::SendPacket(uint64 session_id, NetSerializeBufferRef packet)
{
	NetSession* session = GetSession(session_id);

	if (session == nullptr)
	{
		return false;
	}

	
	SetPacket(packet.GetPtr());

	//SendQueue에 삽입
	packet->AddCnt();
	if (session->_send_buf.Enqueue(packet.GetPtr()) == df_QUEUE_SIZE)
	{
		OnError(dfLOG_LEVEL_SYSTEM, session_id, df_RECVQ_FULL, L"Session Queue Full");
		DisconectInLib(session);
	}


	//SendFlag를 확인 후 off라면 WSASend 호출
	SendPost(session);

	if (AtomicDecrement32(&session->_io_cnt) == 0)
	{
		session->_logic_instance->RegisterRelease(session);
	}

	return true;
}

bool NetService::SendPacketGroup(uint64 session_id, NetSerializeBuffer* packet)
{
	NetSession* session = GetSession(session_id);

	if (session == nullptr)
	{
		return false;
	}

	//Header Setting
	SetPacket(packet);
	packet->AddCnt();
	if (session->_send_buf.Enqueue(packet) == df_QUEUE_SIZE)
	{
		OnError(dfLOG_LEVEL_SYSTEM, session_id, df_RECVQ_FULL, L"Session Queue Full");
		DisconectInLib(session);
	}

	if (AtomicDecrement32(&session->_io_cnt) == 0)
	{
		session->_logic_instance->RegisterRelease(session);
	}

	return true;
}


bool NetService::SendDisconnect(uint64 session_id, NetSerializeBuffer* packet)
{
	NetSession* session = GetSession(session_id);

	if (session == nullptr)
	{
		return false;
	}

	//Header Setting
	SetDisconnectPacket(packet);
	//RefCnt 증가
	packet->AddCnt();
	if (session->_send_buf.Enqueue(packet) == df_QUEUE_SIZE)
	{
		OnError(dfLOG_LEVEL_SYSTEM, session_id, df_RECVQ_FULL, L"Session Queue Full");
		DisconectInLib(session);
	}

	SendPost(session);

	if (AtomicDecrement32(&session->_io_cnt) == 0)
	{
		session->_logic_instance->RegisterRelease(session);
	}

	return true;
}

bool NetService::SendDisconnectGroup(uint64 session_id, NetSerializeBuffer* packet)
{
	NetSession* session = GetSession(session_id);

	if (session == nullptr)
	{
		return false;
	}

	//Header Setting
	SetDisconnectPacket(packet);
	//RefCnt 증가
	packet->AddCnt();
	if (session->_send_buf.Enqueue(packet) == df_QUEUE_SIZE)
	{
		OnError(dfLOG_LEVEL_SYSTEM, session_id, df_RECVQ_FULL, L"Session Queue Full");
		DisconectInLib(session);
	}

	if (AtomicDecrement32(&session->_io_cnt) == 0)
	{
		session->_logic_instance->RegisterRelease(session);
	}

	return true;
}



bool NetService::Disconnect(uint64 session_id)
{
	NetSession* session = GetSession(session_id);

	if (session == nullptr)
		return false;

	do
	{
		if (InterlockedExchange8(&session->_disconnect_flag, 1) == 1)
			break;

		::CancelIoEx((HANDLE)_sessions[static_cast<uint16>(session_id)]._socket, reinterpret_cast<OVERLAPPED*>(&_sessions[static_cast<uint16>(session_id)]._send_overlapped));
		::CancelIoEx((HANDLE)_sessions[static_cast<uint16>(session_id)]._socket, reinterpret_cast<OVERLAPPED*>(&_sessions[static_cast<uint16>(session_id)]._recv_overlapped));

	} while (false);

	if (AtomicDecrement32(&session->_io_cnt) == 0)
	{
		session->_logic_instance->RegisterRelease(session);
	}

	return true;
}


void NetService::ReturnSession(NetSession* session)
{
	if (AtomicDecrement32(&session->_io_cnt) == 0)
	{
		session->_logic_instance->RegisterRelease(session);
	}
}


void NetService::DisconectInLib(NetSession* session)
{
	//Library 내부이므로 io_cnt를 물고 있는 상태이기 때문에 ReleasePost Flag를 체크할 필요가 없다.
	do
	{

		if (InterlockedExchange8(&session->_disconnect_flag, 1) == 1)
			break;

		::CancelIoEx((HANDLE)_sessions[static_cast<uint16>(session->_session_id)]._socket, nullptr);

	} while (false);

}

void NetService::ResetProc(NetSession* session)
{
	FREE_NET_RECV_PACKET(session->_recv_buf);
	
	//Debug 코드
	session->_recv_buf = nullptr;
	closesocket(session->_socket);

	_session_idx_stack.push(static_cast<SHORT>(session->_session_id));

	AtomicDecrement64(&_alloc_session_num);

}



void NetService::ReleasePost(NetSession* session)
{

	memset(&session->_recv_overlapped, 0, sizeof(WSAOVERLAPPED));
	session->_recv_overlapped.type = OverlappedEventType::RELEASE_OVERLAPPED;

	PostQueuedCompletionStatus(_iocp_core._iocp_handle, 0, 0, reinterpret_cast<OVERLAPPED*>(&session->_recv_overlapped));
}

void NetService::ReleaseProc(NetSession* session)
{
	//Solo, Group에 속한 상태라면 콘텐츠 로직 호출
	if(session->_logic_instance != nullptr)
		session->_logic_instance->OnRelease(session->_session_id, session->_instance);

	//Session Data 초기화, SendQueue 직렬화 버퍼 해제, RecvBuffer 반환 등등
	session->SessionNetClear();

	//Session IDX 반환하여 재사용
	_session_idx_stack.push(static_cast<SHORT>(session->_session_id));

	AtomicDecrement32(&_cur_session_num);
	AtomicDecrement64(&_alloc_session_num);

}

NetSession* NetService::GetSession(uint64 session_id)
{
	NetSession* session = &_sessions[static_cast<SHORT>(session_id)];

	//먼저 다른 경우 interlock 하기 전에 나오는 것이 이득이다.

	uint32 io_cnt = AtomicIncrement32(&session->_io_cnt);

	if ((io_cnt & RELEASE_MASK) != 0 || (session->_session_id != session_id))
	{
		if (AtomicDecrement32(&session->_io_cnt) == 0)
		{
			session->_logic_instance->RegisterRelease(session);
		}
		return nullptr;
	}
	return session;
}



void NetService::SendPost(NetSession* session)
{
	//cnt에 대한 고민이 들어가야 하는지 헷갈린다.
	int32 send_err = 0;
	int32 send_cnt;

	//Disconnect시 Return
	if (session->_disconnect_flag == 1)
	{
		return;
	}

	LockFreeQueueStatic<NetSerializeBuffer*>* send_buf = &session->_send_buf;

	while (true)
	{
		if (send_buf->Size() <= 0)
			return;

		if (InterlockedExchange8(&session->_send_flag, 1) == 1)
		{
			return;
		}

		send_cnt = send_buf->Size();

		if (send_cnt <= 0)
			InterlockedExchange8(&session->_send_flag, 0);
		else
			break;
	}


	WSABUF wsabuf[df_SEND_ARR_SIZE];
	NetSerializeBuffer* buffer;

	if (send_cnt > df_SEND_ARR_SIZE)
		send_cnt = df_SEND_ARR_SIZE;

	for (int i = 0; i < send_cnt; i++)
	{
		send_buf->Dequeue(buffer);
		wsabuf[i].buf = buffer->GetPacketPtr();
		wsabuf[i].len = buffer->GetPacketSize();
		session->_send_arr[i] = buffer;
	}

	session->_send_cnt = send_cnt;


	InterlockedAdd(reinterpret_cast<LONG*>(&_observer->_send_msg_tps), send_cnt);

	memset(&session->_send_overlapped, 0, sizeof(WSAOVERLAPPED));

	AtomicIncrement32(&session->_io_cnt);

	//굳이 Transferred와 flag가 변수로 필요하지 않다.
	if (::WSASend(session->_socket, wsabuf, send_cnt, nullptr, 0, &session->_send_overlapped, nullptr) == SOCKET_ERROR)
	{
		send_err = ::WSAGetLastError();

		if (send_err == WSA_IO_PENDING)
		{
			if (session->_disconnect_flag == 1)
			{
				::CancelIoEx((HANDLE)session->_socket, nullptr);
			}
			return;
		}

		AtomicDecrement32(&session->_io_cnt);

		if (send_err == WSAECONNRESET)
			return;

		LIB_LOG(dfLOG_LEVEL_SYSTEM, session->_session_id, send_err, L"WSASend Fail");
	}
	return;
}

void NetService::RecvPost(NetSession* session)
{
	int recv_err = 0;


	if (session->_disconnect_flag == 1)
	{
		return;
	}

	DWORD flags = 0;

	if (session->_recv_buf->GetFreeSize() == 0)
	{
		LIB_LOG(dfLOG_LEVEL_SYSTEM, session->_session_id, df_RECVQ_FULL, L"Session RecvBuffer Full");
		DisconectInLib(session);
		return;
	}

	WSABUF wsabuf;


	wsabuf.buf = session->_recv_buf->GetWritePos();
	wsabuf.len = session->_recv_buf->GetFreeSize();

	memset(&session->_recv_overlapped, 0, sizeof(WSAOVERLAPPED));
	session->_recv_overlapped.type = OverlappedEventType::RECV_OVERLAPPED;

	AtomicIncrement32(&session->_io_cnt);

	if (::WSARecv(session->_socket, &wsabuf, 1, nullptr, &flags, &session->_recv_overlapped, nullptr) == SOCKET_ERROR)
	{
		recv_err = ::WSAGetLastError();

		if (recv_err == WSA_IO_PENDING)
		{
			if (session->_disconnect_flag == 1)
			{
				::CancelIoEx((HANDLE)session->_socket, nullptr);
			}
			return;
		}

		AtomicDecrement32(&session->_io_cnt);

		if (recv_err == WSAECONNRESET)
			return;

		LIB_LOG(dfLOG_LEVEL_SYSTEM, session->_session_id, recv_err, L"WSARecv Fail");
	}
	return;
}
void NetService::AcceptPost()
{
	int32 err_code = 0;

	uint16 session_idx;
	NetSession* session;
	DWORD transferred;

	AtomicIncrement64(&_alloc_session_num);

	//Session MAX를 stack size로 처리하고 있다.
	if (_session_idx_stack.pop(session_idx) == false)
	{
		session = _session_pool->Alloc();
		//소켓 생성 및 RecvBuffer 할당만 이루어진다.
		session->SessionNetTempInit();

		if (_iocp_core.Register(session) == false)
		{
			API_BREAKLOG(dfLOG_LEVEL_SYSTEM, session->_session_id, err_code, L"IOCP Core Socket Register Fail");
		}
	}
	else
	{
		session = &_sessions[session_idx];

		//Data초기화 및 AcceptOverlapped 설정, socket 생성
		session->SessionNetPreInit((session_idx) | ((AtomicIncrement64(&_session_cnt) - 1) << 16));
		if (_iocp_core.Register(session) == false)
		{
			API_BREAKLOG(dfLOG_LEVEL_SYSTEM, session->_session_id, err_code, L"IOCP Core Socket Register Fail");
		}
	}


	if (SocketUtils::AcceptEx(_listen_socket, session->_socket, session->_recv_buf->GetWritePos(), 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &transferred, &session->_recv_overlapped) == false)
	{
		err_code = ::WSAGetLastError();

		if (err_code != WSA_IO_PENDING)
		{
			if (InterlockedAnd8(&_exit_flag, 1) == 1)
			{
				AtomicDecrement64(&_alloc_session_num);
				_session_idx_stack.push(session_idx);
				return;
			}
			LIB_BREAKLOG(dfLOG_LEVEL_SYSTEM, session->_session_id, err_code, L"AcceptEx Fail");
		}

	}
}

bool NetService::AcceptProc(NetSession* session)
{
	int32 err_code = 0;
	SOCKADDR_IN client_addr;
	int32 sock_addr_len = sizeof(SOCKADDR_IN);
	WCHAR ip_buf[IPBUF_SIZE];
	USHORT port;

	AtomicIncrement64(&_observer->_accept_total);
	AtomicIncrement32(&_observer->_accept_tps);
	AtomicIncrement32(&_cur_session_num);

	//listen_socket으로 부터 특성 상속받기
	if (SocketUtils::CopySocketAttribute(session->_socket, _listen_socket) == false)
	{
		API_LOG(dfLOG_LEVEL_SYSTEM, session->_session_id, err_code, L"ClientSocket Copy Attributes Fail");
		return false;
	}

	if (::getpeername(session->_socket, reinterpret_cast<SOCKADDR*>(&client_addr), &sock_addr_len) == SOCKET_ERROR)
	{
		API_LOG(dfLOG_LEVEL_SYSTEM, session->_session_id, err_code, L"ClientSocket Peer Name Fail");
		return false;
	}

	//IP, PORT 가져오는 Util 함수
	NetAddress::GetProcessIPPort(client_addr, ip_buf, port);

	//IP, PORT 관련 Callback 호출
	if (OnConnectionRequest(ip_buf, port) == false)
	{
		return false;
	}

	//io_cnt, time 초기화 (실제 할당 이후 처리되어야 하는 부분) 
	session->SessionNetProcInit();

	//Accept Callback 및 Recv 등록
	if (OnAccept(session->_session_id))
	{
		//Accept 등록 XXXX -> Error
		if (session->_logic_instance == nullptr)
			__debugbreak();

		RecvPost(session);
	}


	AcceptPost();

	return true;
}

void NetService::MaxAcceptProc(NetSession* session)
{
	session->SessionNetTempClear();
	
	_session_pool->Free(session);

	AtomicDecrement64(&_alloc_session_num);
	AcceptPost();

}

void NetService::SendProc(NetSession* session)
{
	int32 send_cnt = session->_send_cnt;
	bool disconnect_flag = false;

	for (int i = 0; i < send_cnt; i++)
	{
		//Disconnect Flag 존재여부 확인
		if (session->_send_arr[i]->_send_disconnect_flag == 1)
			disconnect_flag = true;

		FREE_NET_SEND_PACKET(session->_send_arr[i]);
	}

	session->_send_cnt = 0;

	if (disconnect_flag)
	{
		//Flag가 서 있다면 끊어버린다.
		DisconectInLib(session);
		return;
	}

	InterlockedExchange8(&session->_send_flag, 0);

	SendPost(session);
	
	return;
}


void NetService::RecvProc(NetSession* session, DWORD transferred)
{
	//Library 내부이므로 삭제 신경 X -> Time 측정을 해준다.
	session->_time = GetTickCount64();

	NetSerializeBuffer* packet; 	//Packet을 외부에서 다시 사용하는 경우가 발생할 수 있다. -> 이에 대한 고려가 필요하다. (LAN Server에서의 전송)
	int32 data_size;
	int num = 0;

	//Buffer 수용 크기만큼 들어온다.
	//따라서 RecvBuffer에서 크기를 다 써서 바꿔야하는 경우는 마지막에만 처리하면 된다.

	session->_recv_buf->MoveWritePos(transferred);
	
	LogicInstance* logic_instance = session->GetLogicInstance();

	while (true)
	{
		//모니터링 값
		data_size = session->_recv_buf->GetDataSize();

		//헤더 크기 확인
		if (data_size < sizeof(NetHeader))
		{
			break;
		}

		if (reinterpret_cast<NetHeader*>(session->_recv_buf->GetReadPos())->_code != _packet_code)
		{
			DisconectInLib(session);
			LIB_LOG(dfLOG_LEVEL_SYSTEM, session->_session_id, df_HEADER_CODE_ERROR, L"NetHeader CODE ERROR");
			return;
		}

		//len 길이 가지고 오기
		int packet_len = reinterpret_cast<NetHeader*>(session->_recv_buf->GetReadPos())->_len;

		//packet 길이가 다른 경우 제거
		if (data_size < sizeof(NetHeader) + packet_len)
		{
			if (packet_len > df_MAX_PACKET_LEN ||  packet_len > session->_recv_buf->GetBufferSize() )
			{
				DisconectInLib(session);
				LIB_LOG(dfLOG_LEVEL_SYSTEM, session->_session_id, df_HEADER_LEN_ERROR, L"NetHeader LEN ERROR");
				return;
			}

			break;
		}

		packet = session->_recv_buf->GetNetSerializeBuffer(packet_len);

		if (GetPacket(packet) == false)
		{
			DisconectInLib(session);
			LIB_LOG(dfLOG_LEVEL_SYSTEM, session->_session_id, df_HEADER_CHECKSUM_ERROR, L"NetHeader CheckSum ERROR");
			return;
		}

		//packet에 대한 Ref 증가는 OnRecv에서 처리가 이루어진다.

		logic_instance->RecvMsg(session, packet);

		//RecvPacket Buffer의 경우 패킷이 많이 왔다면 다 사용했을 수 있다. 따라서 교체가 필요
		if (session->_recv_buf->MoveIdxAndPos(packet_len + sizeof(NetHeader)) == false)
		{
			NetRecvPacket* temp_recv_packet = ALLOC_NET_RECV_PACKET();
			//기존에 남은 데이터를 새로 할당받은 RecvPacket의 버퍼에 복사
			temp_recv_packet->CopyRemainData(session->_recv_buf);
			FREE_NET_RECV_PACKET(session->_recv_buf);
			session->_recv_buf = temp_recv_packet;
		}
		num++;
	}

	//만약 남아있는 패킷이 있다면 수신 버퍼가 줄어들기 때문에 재활용한다. + 항상 수신 가능 크기만큼만 들어오기 때문에
	//크기에 한해서 매번 확인후 재활용할 필요는 없다.
	//Readpos가 옮겨 졌다는 것은 사용이 됬다는 것이므로 이를 통해 확인이 가능하다.


	if (session->_recv_buf->Used())
	{
		NetRecvPacket* temp_recv_packet = ALLOC_NET_RECV_PACKET();
		//기존에 남은 데이터를 새로 할당받은 RecvPacket의 버퍼에 복사하는 함수
		temp_recv_packet->CopyRemainData(session->_recv_buf);
		FREE_NET_RECV_PACKET(session->_recv_buf);
		session->_recv_buf = temp_recv_packet;
	}

	InterlockedAdd(reinterpret_cast<LONG*>(&_observer->_recv_msg_tps), num);

	RecvPost(session);

	return;
}



#pragma warning (default : 4700)
#pragma warning (default : 4703)