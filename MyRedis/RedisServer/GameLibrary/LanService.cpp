#include "pch.h"
#include "LanService.h"
#include "SocketUtils.h"
#include "LanSession.h"
#include "LogUtils.h"
#include "Lock.h"
#include "LockFreeQueue.h"
#include "TextParser.h"
#include "LanRecvPacket.h"
#include "LanSendPacket.h"
#include "LockFreeQueueStatic.h"
#include "ThreadManager.h"

#pragma warning (disable : 4700)
#pragma warning (disable : 4703)


LanService::LanService() : _session_cnt(0), _type(ServiceType::LanServerType), _sessions(nullptr), _session_pool(nullptr)
{
	_thread_manager = new ThreadManager;
}

LanService::~LanService()
{
	delete[] _sessions;
	delete _session_pool;
	delete _thread_manager;
}

bool LanService::Init(const WCHAR* ip, SHORT port, int32 concurrent_thread_num, int32 max_thread_num, bool direct_io, bool nagle_onoff, int32 max_session_num, int32 accept_job_cnt, int32 send_time, int32 monitor_time , int32 backlog_size)
{
	int32 err_code = 0;
	size_t size;

	_port = port;
	wcstombs_s(&size, _ip, IPBUF_SIZE, ip, SIZE_MAX);

	WCHAR* log_buff;
	_send_post_overlapped.type = OverlappedEventType::SENDPOST_OVERLAPPED;
	_send_post_overlapped.instance = nullptr;

	_session_pool = new LockFreeObjectPool<LanSession, false>;

	//서버 타입이 및 시작 처리 -> 나중에 손보자
	_exit_flag = false;

	_max_session_num = max_session_num;
	_cur_session_num = 0;
	_alloc_session_num = 0;

	//모아서 보내기 여부 판단 0이면 안모음
	_send_time = send_time;

	NetAddress::SetAddress(_server_addr, ip, port);

	_sessions = new LanSession[_max_session_num];

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
		_sessions[i]._type = ServiceType::LanServerType;
	}

	_observer.ObserverInit(monitor_time);

	//Monitor Start
	if (monitor_time != 0)
	{
		CreateObserverThread(monitor_time);
		log_buff = g_logutils->GetLogBuff(L"Monitor Thread 시작! [MonitorTime : %d]", monitor_time);
		g_logutils->Log(NET_LOG_DIR, NET_FILE_NAME, log_buff);
	}

	AcceptStart(direct_io, nagle_onoff, backlog_size, accept_job_cnt);

	return true;
}

void LanService::Start()
{
	_thread_manager->Start();
}

void LanService::Stop()
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

	for (int cnt = 0; cnt < _iocp_core._thread_nums; cnt++)
	{
		PostQueuedCompletionStatus(_iocp_core._iocp_handle, 0, 0, NULL);
	}

	OnExit();

	_thread_manager->Join();
}

void LanService::CreateIOThread(int concurrent_thread_nums, int max_thread_nums)
{
	_iocp_core.IOInit(this, concurrent_thread_nums, max_thread_nums);

	for (int i = 0; i < max_thread_nums; i++)
	{
		_thread_manager->Launch(IocpCore::LanIOThread, this, ServiceType::LanServerType, MonitorInfoType::IOInfoType);
	}
}

void LanService::CreateObserverThread(int32 monitor_time)
{
	_thread_manager->Launch(LanObserver::LanObserverThread, this, ServiceType::LanServerType, MonitorInfoType::NoneInfo);
}

void LanService::CreateTimeOutThread(int32 timeout_time)
{
	if (timeout_time != 0)
	{
		_timeout_time = timeout_time;
		_thread_manager->Launch(TimeOutThread, this, ServiceType::LanServerType, MonitorInfoType::NoneInfo);
	}
}

unsigned int LanService::TimeOutThread(void* service_ptr)
{
	LanService* service = reinterpret_cast<LanService*>(service_ptr);
	service->ObserverThreadLoop();
	return 0;
}

void LanService::IOThreadLoop()
{
	int32 err_code = 0;
	DWORD gqcs_transferred_dw;
	LanSession* session;
	ULONG_PTR key;
	OverlappedEvent* overlapped;

	_thread_manager->Wait();

	while (true)
	{
		//int32 err_code;
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

		session = reinterpret_cast<LanSession*>(overlapped->instance);

		switch (overlapped->type)
		{
		case OverlappedEventType::ACCEPT_OVERLAPPED:
			if (!iocp_ret || AcceptProc(session) == false)
			{
				ResetProc(session);
				AcceptPost();
				continue;
			}
			break;
		case OverlappedEventType::MAXACCEPT_OVERLAPPED:
			session = reinterpret_cast<LanSession*>(overlapped->instance);
			MaxAcceptProc(session);
			continue;
		case OverlappedEventType::RELEASE_OVERLAPPED:
			ReleaseProc(session);
			continue;
		case OverlappedEventType::SEND_OVERLAPPED:
			if (iocp_ret)
			{
				SendProc(session);
			}
			break;
		case OverlappedEventType::RECV_OVERLAPPED:
			if (iocp_ret && gqcs_transferred_dw != 0)
			{
				RecvProc(session, gqcs_transferred_dw);
			}
			break;
		case OverlappedEventType::SENDPOST_OVERLAPPED:
			SendPostOnly();
			continue;
		default:
			LIB_SYSTEM_BREAKLOG(dfLOG_LEVEL_SYSTEM, err_code, L"OVERLAPPED TYPE ERROR");
			break;
		}

		if (AtomicDecrement32(&session->_io_cnt) == 0)
		{
			ReleasePost(session);
		}
	}

	return;
}

void LanService::ObserverThreadLoop()
{
	char ip[20];
	USHORT port;
	NetAddress::GetProcessIPPort(_server_addr, ip, port);

	_thread_manager->Wait();

	while (_exit_flag == 0)
	{
		Sleep(_observer._monitor_time);

		_observer.UpdateLibData();


		OnMonitor();
	}
}

void LanService::TimeOutThreadLoop()
{
	_thread_manager->Wait();

	uint64 session_id;
	uint64 cur_time;

	int32 timeout_time = _timeout_time;

	while (_exit_flag == 0)
	{
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
			if (_sessions[i]._time + timeout_time < cur_time)
			{
				//Disconnect 호출 시 현재 session의 session_id가 일치하는지 여부를 다시 한번 확인한다.
				OnTimeOut(session_id);
				WCHAR* log_buff = g_logutils->GetLogBuff(L"TimeOut Session [SessionID : %I64u]", session_id);
				g_logutils->Log(NET_LOG_DIR, NET_FILE_NAME, log_buff);
			}
		}

	}
	

}

void LanService::PrintLibraryConsole()
{
	printf("========================================================================================\n");
	printf("LAN SERVER LiBRARY V1218-2 [IP: %s PORT: %hu MAX_THREAD_NUM: %d CONCURENT_THREAD: %d]\n", _ip, _port, _iocp_core._thread_nums, _iocp_core._concurrent_thread_nums);
	printf("----------------------------------------------------------------------------------------\n");
	printf("Session Num : %d  MAX Session : %d  Alloc Session : %I64u\n", _cur_session_num, _max_session_num, _alloc_session_num);
	printf("Accept        [TPS: %d AVG: %I64u TOTAL: %I64u] \n", _observer._accept_tps, (_observer._accept_total / _observer._out_tick), _observer._accept_total);
	printf("SendMSG       [TPS: %d AVG: %I64u]\n", _observer._out_send_msg_tps, _observer._send_msg_avg / _observer._out_tick);
	printf("RecvMSG       [TPS: %d AVG: %I64u]\n", _observer._out_recv_msg_tps, _observer._recv_msg_avg / _observer._out_tick);
	printf("SendChunk     [Cacpacity: %u Use: %u Release: %u]\n", LanSendPacket::g_send_lan_packet_pool.GetChunkCapacityCount(), LanSendPacket::g_send_lan_packet_pool.GetChunkUseCount(), LanSendPacket::g_send_lan_packet_pool.GetChunkReleaseCount());
	printf("SendChunkNode [Use: %u Release: %u ChunkSize: %d]\n", LanSendPacket::g_send_lan_packet_pool.GetChunkNodeUseCount(), LanSendPacket::g_send_lan_packet_pool.GetChunkNodeReleaseCount(), MAX_CHUNK_SIZE);
	printf("RecvChunk     [Cacpacity: %u Use: %u Release: %u]\n", LanRecvPacket::g_recv_lan_packet_pool.GetChunkCapacityCount(), LanRecvPacket::g_recv_lan_packet_pool.GetChunkUseCount(), LanRecvPacket::g_recv_lan_packet_pool.GetChunkReleaseCount());
	printf("RecvChunkNode [Use: %u Release: %u ChunkSize: %d]\n", LanRecvPacket::g_recv_lan_packet_pool.GetChunkNodeUseCount(), LanRecvPacket::g_recv_lan_packet_pool.GetChunkNodeReleaseCount(), MAX_CHUNK_SIZE);
	printf("----------------------------------------------------------------------------------------\n");

	OnMonitor();
}

void LanService::SendPostCall()
{
	PostQueuedCompletionStatus(_iocp_core._iocp_handle, 0, 0, &_send_post_overlapped);
}

bool LanService::Disconnect(uint64 session_id)
{
	LanSession* session = GetSession(session_id);

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
		ReleasePost(session);
	}

	return true;
}

bool LanService::SendDisconnect(uint64 session_id)
{
	return false;
}

bool LanService::SendPacket(uint64 session_id, LanSerializeBuffer* packet)
{
	LanSession* session = GetSession(session_id);

	if (session == nullptr)
	{
		return false;
	}

	//헤더 설정
	reinterpret_cast<LanHeader*>(packet->GetPacketPtr())->_len = packet->GetDataSize();
	//Header Setting
	packet->AddCnt();
	if (session->_send_buf.Enqueue(packet) == -1)
	{
		OnError(dfLOG_LEVEL_SYSTEM, session_id, df_RECVQ_FULL, L"Session Queue Full");
		DisconectInLib(session);
	}

	if (_send_time == 0)
		SendPost(session);

	if (AtomicDecrement32(&session->_io_cnt) == 0)
	{
		ReleasePost(session);
	}

	return true;
}

bool LanService::SendPacket(uint64 session_id, LanSerializeBufferRef packet)
{
	LanSession* session = GetSession(session_id);

	if (session == nullptr)
	{
		return false;
	}

	//헤더 설정
	reinterpret_cast<LanHeader*>(packet->GetPacketPtr())->_len = packet->GetDataSize();
	//Header Setting
	packet->AddCnt();
	if (session->_send_buf.Enqueue(packet.GetPtr()) == -1)
	{
		OnError(dfLOG_LEVEL_SYSTEM, session_id, df_RECVQ_FULL, L"Session Queue Full");
		DisconectInLib(session);
	}

	if (_send_time == 0)
		SendPost(session);

	if (AtomicDecrement32(&session->_io_cnt) == 0)
	{
		ReleasePost(session);
	}

	return true;
}


void LanService::SendPostOnly()
{
	int cnt = 0;

	for (int idx = 0; idx < _max_session_num; idx++)
	{
		uint64 session_id = _sessions[idx]._session_id;

		if (session_id == UINT16_MAX)
			continue;

		LanSession* session = GetSession(session_id);

		if (session == nullptr)
			continue;

		if (session->_send_buf.Size() > 0)
			SendPost(session);

		ReturnSession(session);
	}
}

void LanService::AcceptStart(bool direct_io, bool nagle_on, int32 backlog_size, int32 accept_job_cnt)
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

	if (direct_io)
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
	}

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

void LanService::SendPost(LanSession* session)
{
	//cnt에 대한 고민이 들어가야 하는지 헷갈린다.
	int32 send_err = 0;
	int32 send_cnt;


	if (session->_disconnect_flag == 1)
	{
		return;
	}

	LockFreeQueueStatic<LanSerializeBuffer*>* send_buf = &session->_send_buf;

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
	LanSerializeBuffer* buffer;

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

	InterlockedAdd(reinterpret_cast<LONG*>(&_observer._send_msg_tps), send_cnt);

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

void LanService::SendProc(LanSession* session)
{
	int32 send_cnt = session->_send_cnt;

	for (int i = 0; i < send_cnt; i++)
	{
		FREE_LAN_SEND_PACKET(session->_send_arr[i]);
	}

	//굳이 필요없을 듯?? 아니지 무조건 필요하지...
	session->_send_cnt = 0;

	InterlockedExchange8(&session->_send_flag, 0);

	if (_send_time == 0)
	{
		SendPost(session);
	}
	return;
}

void LanService::RecvPost(LanSession* session)
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
		Disconnect(session->_session_id);
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

void LanService::RecvProc(LanSession* session, DWORD transferred)
{
	session->_time = GetTickCount64();
	LanSerializeBuffer* packet; 	//Packet을 외부에서 다시 사용하는 경우가 발생할 수 있다. -> 이에 대한 고려가 필요하다. (LAN Server에서의 전송)
	int32 data_size;
	int num = 0;

	//ReSize 제대로 추가하기
	session->_recv_buf->MoveWritePos(transferred);

	while (true)
	{
		//모니터링 값
		data_size = session->_recv_buf->GetDataSize();

		//헤더 크기 확인
		if (data_size < sizeof(LanHeader))
		{
			break;
		}

		//len 길이 가지고 오기
		int packet_len = reinterpret_cast<LanHeader*>(session->_recv_buf->GetReadPos())->_len;

		//packet 길이가 다른 경우 제거
		if (packet_len > df_MAX_PACKET_LEN)
		{
			DisconectInLib(session);
			LIB_LOG(dfLOG_LEVEL_SYSTEM, session->_session_id, df_HEADER_LEN_ERROR, L"LanHeader LEN ERROR");
			return;
		}

		if (data_size < sizeof(LanHeader) + packet_len)
		{
			break;
		}

		packet = session->_recv_buf->GetLanSerializeBuffer(packet_len);

		//packet에 대한 Ref 증가는 OnRecv에서 처리가 이루어진다.
		OnRecvMsg(session->_session_id, packet);

		//RecvPacket Buffer를 몇개 둘것인가를 결정해야 한다.
		if (session->_recv_buf->MoveIdxAndPos(packet_len + sizeof(LanHeader)) == false)
		{
			//기존에 recv_buf를 지역 변수로 할당했을 때 할당한 지역변수를 변경하지 않고 계속 사용하는 문제가 발생했다.
			LanRecvPacket* temp_recv_packet = ALLOC_LAN_RECV_PACKET();
			temp_recv_packet->CopyRemainData(session->_recv_buf);
			FREE_LAN_RECV_PACKET(session->_recv_buf);
			session->_recv_buf = temp_recv_packet;
		}
		num++;
	}

	if (session->_recv_buf->Used())
	{
		LanRecvPacket* temp_recv_packet = ALLOC_LAN_RECV_PACKET();
		temp_recv_packet->CopyRemainData(session->_recv_buf);
		FREE_LAN_RECV_PACKET(session->_recv_buf);
		session->_recv_buf = temp_recv_packet;
	}

	InterlockedAdd(reinterpret_cast<LONG*>(&_observer._recv_msg_tps), num);
	RecvPost(session);

	return;
}

void LanService::AcceptPost()
{
	int32 err_code = 0;
	DWORD transferred;
	uint16 session_idx;
	LanSession* session;

	AtomicIncrement64(&_alloc_session_num);

	//Session MAX를 stack size로 처리하고 있다.
	if (_session_idx_stack.pop(session_idx) == false)
	{
		session = _session_pool->Alloc();
		session->SessionNetTempInit();
	}
	else
	{
		session = &_sessions[session_idx];
		session->_recv_buf = ALLOC_LAN_RECV_PACKET();
		session->_session_id = (session_idx) | ((AtomicIncrement64(&_session_cnt) - 1) << 16);
		session->_socket = SocketUtils::CreateSocket();

		if (_iocp_core.Register(session) == false)
		{
			API_BREAKLOG(dfLOG_LEVEL_SYSTEM, session->_session_id, err_code, L"IOCP Core Socket Register Fail");
		}

		memset(&session->_recv_overlapped, 0, sizeof(WSAOVERLAPPED));
		session->_recv_overlapped.type = OverlappedEventType::ACCEPT_OVERLAPPED;
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
			//AcceptPost(session, session_idx, session_cnt);
		}

	}
}

void LanService::MaxAcceptProc(LanSession* session)
{
	session->SessionNetTempClear();
	_session_pool->Free(session);
	AtomicDecrement64(&_alloc_session_num);

	AcceptPost();
}

bool LanService::AcceptProc(LanSession* session)
{
	int32 err_code = 0;
	SOCKADDR_IN client_addr;
	int32 sock_addr_len = sizeof(SOCKADDR_IN);
	WCHAR ip_buf[IPBUF_SIZE];
	USHORT port;

	AtomicIncrement64(&_observer._accept_total);
	AtomicIncrement32(&_observer._accept_tps);

	//아래에는 RecvBuffer 및 SessionId에 대해서만 초기화가 이루어져 있다.
	AtomicIncrement32(&_cur_session_num);
	//listen_socket으로 부터 특성 상속받기 -> 연결 확인으로도 사용이 가능하다.
	//연결 자체가 안된 것이지만 재사용을 위한 DisconnectEx를 호출해주어야만 한다.
	if (SocketUtils::CopySocketAttribute(session->_socket, _listen_socket) == false)
	{
		API_LOG(dfLOG_LEVEL_SYSTEM, session->_session_id, err_code, L"ClientSocket Copy Attributes Fail");
		return false;
	}

	//위와 마찬가지이다.
	//listen_socket으로 부터 특성 상속받기 -> 연결 확인으로도 사용이 가능하다.
	if (::getpeername(session->_socket, reinterpret_cast<SOCKADDR*>(&client_addr), &sock_addr_len) == SOCKET_ERROR)
	{
		API_LOG(dfLOG_LEVEL_SYSTEM, session->_session_id, err_code, L"ClientSocket Peer Name Fail");
		return false;
	}

	//아래부터는 연결은 성공한 것이다.

	NetAddress::GetProcessIPPort(client_addr, ip_buf, port);

	if (OnConnectionRequest(ip_buf, port) == false)
	{
		return false;
	}

	//실제 초기화하는 부분 
	session->SessionProcInit();

	if (OnAccept(session->_session_id))
	{
		RecvPost(session);
	}

	AcceptPost();

	return true;
}

void LanService::ReleasePost(LanSession* session)
{
	if (InterlockedCompareExchange(reinterpret_cast<long*>(&session->_io_cnt), RELEASE_MASK, 0) != 0)
		return;

	memset(&session->_recv_overlapped, 0, sizeof(WSAOVERLAPPED));
	session->_recv_overlapped.type = OverlappedEventType::RELEASE_OVERLAPPED;

	PostQueuedCompletionStatus(_iocp_core._iocp_handle, 0, 0, reinterpret_cast<OVERLAPPED*>(&session->_recv_overlapped));
}



void LanService::ResetProc(LanSession* session)
{
	FREE_LAN_RECV_PACKET(session->_recv_buf);
	closesocket(session->_socket);

	_session_idx_stack.push(static_cast<SHORT>(session->_session_id));

	AtomicDecrement64(&_alloc_session_num);
}

void LanService::ReleaseProc(LanSession* session)
{

	uint64 session_id = session->_session_id;

	OnRelease(session_id);

	session->SessionLanClear();
	closesocket(session->_socket);

	_session_idx_stack.push(static_cast<SHORT>(session_id));

	AtomicDecrement32(&_cur_session_num);
	AtomicDecrement64(&_alloc_session_num);
}

LanSession* LanService::GetSession(uint64 session_id)
{
	LanSession* session = &_sessions[static_cast<SHORT>(session_id)];

	//먼저 다른 경우 interlock 하기 전에 나오는 것이 이득이다.

	uint32 io_cnt = AtomicIncrement32(&session->_io_cnt);

	if ((io_cnt & RELEASE_MASK) != 0 || (session->_session_id != session_id))
	{
		if (AtomicDecrement32(&session->_io_cnt) == 0)
		{
			ReleasePost(session);
		}

		return nullptr;
	}
	return session;
}

void LanService::ReturnSession(LanSession* session)
{
	if (AtomicDecrement32(&session->_io_cnt) == 0)
	{
		ReleasePost(session);
	}
}

void LanService::DisconectInLib(LanSession* session)
{
	//Library 내부이므로 io_cnt를 물고 있는 상태이기 때문에 ReleasePost Flag를 체크할 필요가 없다.
	do
	{
		if (InterlockedExchange8(&session->_disconnect_flag, 1) == 1)
			break;

		::CancelIoEx((HANDLE)_sessions[static_cast<uint16>(session->_session_id)]._socket, nullptr);

	} while (false);

}


#pragma warning (default : 4700)
#pragma warning (default : 4703)