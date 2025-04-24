#include "pch.h"
#include "NetClient.h"
#include "NetSendPacket.h"
#include "NetRecvPacket.h"
#include "SocketUtils.h"

bool NetClient::Init(const WCHAR* ip, SHORT port, int32 concurrent_thread_num, int32 max_thread_num, uint8 packet_code, uint8 packet_key, bool direct_io, bool nagle_on, int32 send_time, int32 monitor_time)
{
	int32 err_code = 0;
	WCHAR* log_buff;

	//Send �ʱ�ȭ
	_send_post_overlapped.type = OverlappedEventType::SENDPOST_OVERLAPPED;
	_send_post_overlapped.instance = nullptr;


	//��Ƽ� ������ ���� �Ǵ� 0�̸� �ȸ���
	_send_time = send_time;
	_packet_code = packet_code;
	_packet_key = packet_key;

	//server_addr ��� �� IP, PORT ���
	NetAddress::SetAddress(_connect_sever_addr, ip, port);
	NetAddress::GetProcessIPPort(_connect_sever_addr, _ip, _port);

	//IO Start -> IOCP ���� 
	CreateIOThread(concurrent_thread_num, max_thread_num);

	log_buff = g_logutils->GetLogBuff(L"------ Net Client ���� [CONNECT SERVER IP: %s, Port: %hd] ------", ip, port);
	g_logutils->Log(NET_LOG_DIR, NET_FILE_NAME, log_buff);

	log_buff = g_logutils->GetLogBuff(L"IO Thread ����! [ConcurrentThread : %d, MaxThreadNum : %d]", concurrent_thread_num, max_thread_num);
	g_logutils->Log(NET_LOG_DIR, NET_FILE_NAME, log_buff);

	_exit_io_thread_cnt = 0;

	//Observer �ʱ�ȭ
	_send_msg_tps = 0;
	_recv_msg_tps = 0;
	_send_msg_tick = 0;
	_recv_msg_tick = 0;
	_send_msg_avg = 0;
	_recv_msg_avg = 0;

	ConnectStart(direct_io, nagle_on);

	return true;
}


void NetClient::Start()
{
	_thread_manager->Start();
}

void NetClient::Stop()
{
	Disconnect();

	for (int idx = 0; idx < _iocp_core._thread_nums; idx++)
	{
		PostQueuedCompletionStatus(_iocp_core._iocp_handle, 0, 0, NULL);
	}

	while (_exit_io_thread_cnt != _iocp_core._thread_nums)
	{

	}

	OnExit();

	_thread_manager->Join();
}

void NetClient::CreateIOThread(int concurrent_thread_nums, int max_thread_nums)
{
	_iocp_core.IOInit(this, concurrent_thread_nums, max_thread_nums);

	for (int i = 0; i < max_thread_nums; i++)
	{
		_thread_manager->Launch(IocpCore::NetClientIOThread, this, ServiceType::NetClientType, MonitorInfoType::IOInfoType);
	}

}

void NetClient::IOThreadLoop()
{
	int32 err_code = 0;
	DWORD gqcs_transferred_dw;
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

		if (overlapped == nullptr && gqcs_transferred_dw == 0)
		{
			if (key == 0)
			{
				if (iocp_ret == true)
				{
					LIB_SYSTEM_LOG(dfLOG_LEVEL_SYSTEM, 0, L"IOCP Thread ����");
					break;
				}
				else
				{
					err_code = WSAGetLastError();
					LIB_SYSTEM_BREAKLOG(dfLOG_LEVEL_SYSTEM, err_code, L"IOCP Thread Fail");
					return;
				}

			}
		}

		switch (overlapped->type)
		{
		case OverlappedEventType::RELEASE_OVERLAPPED:
			ReleaseProc();
			continue;
		case OverlappedEventType::SEND_OVERLAPPED:
			if (!iocp_ret)
			{
				break;
			}
			SendProc();
			break;
		case OverlappedEventType::RECV_OVERLAPPED:
			if (!iocp_ret || gqcs_transferred_dw == 0)
			{
				break;
			}
			RecvProc(gqcs_transferred_dw);
			break;
		case OverlappedEventType::SENDPOST_OVERLAPPED:
			SendPostOnly();
			continue;
		default:
			OnError(dfLOG_LEVEL_SYSTEM, err_code, L"OVERLAPPED TYPE ERROR");
			break;
		}

		if (AtomicDecrement32(&_session._io_cnt) == 0)
		{
			ReleasePost();
		}
	}


	AtomicIncrement32(&_exit_io_thread_cnt);

	return;
}

void NetClient::ConnectStart(bool direct_io, bool nagle_on)
{
	int32 err_code = 0;

	//IOCP�� IO Thread �����ÿ� �����ȴ�. 
	_session._socket = SocketUtils::CreateSocket();

	if (_session._socket == INVALID_SOCKET)
	{
		err_code = WSAGetLastError();
		OnError(dfLOG_LEVEL_SYSTEM, err_code, L"CreateSocket Fail");
		__debugbreak();
	}
	OnError(dfLOG_LEVEL_SYSTEM, err_code, L"CreateSocket Success");


	if (_iocp_core.Register(&_session) == false)
	{
		err_code = WSAGetLastError();
		OnError(dfLOG_LEVEL_SYSTEM, err_code, L"IOCP Core Socket Register Fail");
		__debugbreak();
	}

	if (SocketUtils::SetLinger(_session._socket, true, 0) == false)
	{
		err_code = WSAGetLastError();
		OnError(dfLOG_LEVEL_SYSTEM, err_code, L"SetLinger Fail [ErrCode:%d]");
		__debugbreak();
	}
	OnError(dfLOG_LEVEL_SYSTEM, err_code, L"SetLinger Success");

	if (direct_io)
	{
		if (SocketUtils::SetSendBufferSize(_session._socket, 0) == false)
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
		if (SocketUtils::SetNagle(_session._socket, true) == false)
		{
			API_SYSTEM_BREAKLOG(dfLOG_LEVEL_SYSTEM, err_code, L"SetNagle OFF Fail [ErrCode:%d]");
		}
		API_SYSTEM_LOG(dfLOG_LEVEL_SYSTEM, err_code, L"SetNagle OFF Success");
	}
	else
	{
		API_SYSTEM_LOG(dfLOG_LEVEL_SYSTEM, err_code, L"SetNagle ON Success");
	}

	//Connect ó��
	if (SocketUtils::Connect(_session._socket, _connect_sever_addr) == false)
	{
		err_code = WSAGetLastError();
		OnError(dfLOG_LEVEL_SYSTEM, err_code, L"Listen Fail [ErrCode:%d]");
		__debugbreak();
	}

	//IO Cnt ���� �尣��. Connect ó���� send �ϸ鼭 0�� �Ǵ� ���� ����
	_session.SessionProcInit();

	OnConnect();

	//Recv�� ���� 1 ���� -> Recv ���
	RecvPost();

	//ó�� �� 0���� ����� �����Ѵ�.
	if (AtomicDecrement32(&_session._io_cnt) == 0)
	{
		//�ٷ� ������ �����ϱ� ������ �����Ƽ� ����...
		ReleasePost();
	}

	OnError(dfLOG_LEVEL_SYSTEM, err_code, L"Connect Success");
}

void NetClient::PrintClientConsole()
{
	_send_msg_avg += _send_msg_tps;
	_recv_msg_avg += _recv_msg_tps;
	++_recv_msg_tick;
	++_send_msg_tick;

	printf("========================================================================================\n");
	printf("Net CLIENT LIB V1218-2 [Connected IP: %s PORT: %hu THREAD_NUM: %d CONCURENT_THREAD: %d]\n", _ip, _port, _iocp_core._thread_nums, _iocp_core._concurrent_thread_nums);
	printf("----------------------------------------------------------------------------------------\n");
	printf("SendMSG       [TPS: %d AVG: %I64u]\n", _send_msg_tps, _send_msg_avg / _send_msg_tick);
	printf("RecvMSG       [TPS: %d AVG: %I64u]\n", _recv_msg_tps, _recv_msg_avg / _recv_msg_tick);
	printf("SendChunk     [Cacpacity: %u Use: %u Release: %u]\n", NetSendPacket::g_send_net_packet_pool.GetChunkCapacityCount(), NetSendPacket::g_send_net_packet_pool.GetChunkUseCount(), NetSendPacket::g_send_net_packet_pool.GetChunkReleaseCount());
	printf("SendChunkNode [Use: %u Release: %u ChunkSize: %d]\n", NetSendPacket::g_send_net_packet_pool.GetChunkNodeUseCount(), NetSendPacket::g_send_net_packet_pool.GetChunkNodeReleaseCount(), MAX_CHUNK_SIZE);
	printf("RecvChunk     [Cacpacity: %u Use: %u Release: %u]\n", NetRecvPacket::g_recv_net_packet_pool.GetChunkCapacityCount(), NetRecvPacket::g_recv_net_packet_pool.GetChunkUseCount(), NetRecvPacket::g_recv_net_packet_pool.GetChunkReleaseCount());
	printf("RecvChunkNode [Use: %u Release: %u ChunkSize: %d]\n", NetRecvPacket::g_recv_net_packet_pool.GetChunkNodeUseCount(), NetRecvPacket::g_recv_net_packet_pool.GetChunkNodeReleaseCount(), MAX_CHUNK_SIZE);
	printf("----------------------------------------------------------------------------------------\n");

	AtomicExchange32ToZero(&_send_msg_tps);
	AtomicExchange32ToZero(&_recv_msg_tps);
}

//�����ϴٸ� �ٷ� ����
bool NetClient::SendPacket(NetSerializeBuffer* packet)
{
	//���������� Ȯ������ ������ �����Ǿ��� �� �ִ�.
	if (GetSession() == false)
		return false;

	//Header Setting
	SetPacket(packet);
	packet->AddCnt();

	if (_session._send_buf.Enqueue(packet) == df_QUEUE_SIZE)
	{
		OnError(dfLOG_LEVEL_SYSTEM, df_RECVQ_FULL, L"NetClient Session Send Queue Full");
		DisconectInLib();
	}

	SendPost();

	if (AtomicDecrement32(&_session._io_cnt) == 0)
	{
		ReleasePost();
	}

	return true;
}

bool NetClient::SendPacket(NetSerializeBufferRef packet)
{
	if (GetSession() == false)
		return false;

	//Header Setting
	SetPacket(packet.GetPtr());
	packet->AddCnt();
	if (_session._send_buf.Enqueue(packet.GetPtr()) == df_QUEUE_SIZE)
	{
		OnError(dfLOG_LEVEL_SYSTEM, df_RECVQ_FULL, L"NetClient Session Send Queue Full");
		DisconectInLib();
	}

	SendPost();

	if (AtomicDecrement32(&_session._io_cnt) == 0)
	{
		ReleasePost();
	}

	return true;
}

//Enqueue ��
void NetClient::SendPush(NetSerializeBuffer* packet)
{
	if (GetSession() == false)
		return;

	SetPacket(packet);
	packet->AddCnt();

	if (_session._send_buf.Enqueue(packet) == df_QUEUE_SIZE)
	{
		OnError(dfLOG_LEVEL_SYSTEM, df_RECVQ_FULL, L"NetClient Session Send Queue Full");
		DisconectInLib();
	}

	if (AtomicDecrement32(&_session._io_cnt) == 0)
	{
		ReleasePost();
	}

	return;
}

bool NetClient::Disconnect()
{
	if (GetSession() == false)
		return false;

	do
	{
		if (InterlockedExchange8(&_session._disconnect_flag, 1) == 1)
			break;

		::CancelIoEx((HANDLE)_session._socket, reinterpret_cast<OVERLAPPED*>(&_session._send_overlapped));
		::CancelIoEx((HANDLE)_session._socket, reinterpret_cast<OVERLAPPED*>(&_session._recv_overlapped));
	} while (false);

	if (AtomicDecrement32(&_session._io_cnt) == 0)
	{
		ReleasePost();
	}

	return true;
}

bool NetClient::SendDisconnect()
{
	return false;
}

//���� �������� Send
void NetClient::SendPostCall()
{
	int32 err_code;

	if (PostQueuedCompletionStatus(_iocp_core._iocp_handle, 0, 0, &_send_post_overlapped) == false)
	{
		err_code = WSAGetLastError();
		OnError(dfLOG_LEVEL_SYSTEM, err_code, L"SendPostCall PQCS Fail");
		__debugbreak();
	}

}

void NetClient::SendPost()
{
	//cnt�� ���� ����� ���� �ϴ��� �򰥸���.
	int32 send_err = 0;
	int32 send_cnt;

	if (_session._disconnect_flag == 1)
	{
		return;
	}

	LockFreeQueueStatic<NetSerializeBuffer*>* send_buf = &_session._send_buf;

	while (true)
	{
		if (send_buf->Size() <= 0)
			return;

		if (InterlockedExchange8(&_session._send_flag, 1) == 1)
		{
			return;
		}

		send_cnt = send_buf->Size();

		if (send_cnt <= 0)
			InterlockedExchange8(&_session._send_flag, 0);
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
		_session._send_arr[i] = buffer;
	}

	_session._send_cnt = send_cnt;

	InterlockedAdd(reinterpret_cast<LONG*>(&_send_msg_tps), send_cnt);

	memset(&_session._send_overlapped, 0, sizeof(WSAOVERLAPPED));
	AtomicIncrement32(&_session._io_cnt);

	//���� Transferred�� flag�� ������ �ʿ����� �ʴ�.
	if (::WSASend(_session._socket, wsabuf, send_cnt, nullptr, 0, &_session._send_overlapped, nullptr) == SOCKET_ERROR)
	{
		send_err = ::WSAGetLastError();

		if (send_err == WSA_IO_PENDING)
		{
			if (_session._disconnect_flag == 1)
			{
				::CancelIoEx((HANDLE)_session._socket, nullptr);
			}
			return;
		}

		AtomicDecrement32(&_session._io_cnt);

		if (send_err == WSAECONNRESET)
			return;

		OnError(dfLOG_LEVEL_SYSTEM, send_err, L"NetClient WSASend Fail");
	}
	return;

}

void NetClient::SendProc()
{
	int32 send_cnt = _session._send_cnt;

	for (int i = 0; i < send_cnt; i++)
	{
		FREE_NET_SEND_PACKET(_session._send_arr[i]);
	}

	_session._send_cnt = 0;
	InterlockedExchange8(&_session._send_flag, 0);

	//Send Time�� 0�� ��츸 Post
	if (_send_time == 0)
	{
		SendPost();
	}

	return;
}

void NetClient::RecvPost()
{
	int recv_err = 0;

	if (_session._disconnect_flag == 1)
	{
		return;
	}

	DWORD flags = 0;

	if (_session._recv_buf->GetFreeSize() == 0)
	{
		OnError(dfLOG_LEVEL_SYSTEM, df_RECVQ_FULL, L"Session RecvBuffer Full");
		DisconectInLib();
		return;
	}

	WSABUF wsabuf;

	wsabuf.buf = _session._recv_buf->GetWritePos();
	wsabuf.len = _session._recv_buf->GetFreeSize();

	memset(&_session._recv_overlapped, 0, sizeof(WSAOVERLAPPED));
	_session._recv_overlapped.type = OverlappedEventType::RECV_OVERLAPPED;

	AtomicIncrement32(&_session._io_cnt);

	if (::WSARecv(_session._socket, &wsabuf, 1, nullptr, &flags, &_session._recv_overlapped, nullptr) == SOCKET_ERROR)
	{
		recv_err = ::WSAGetLastError();


		if (recv_err == WSA_IO_PENDING)
		{
			if (_session._disconnect_flag == 1)
			{
				::CancelIoEx((HANDLE)_session._socket, nullptr);
			}
			return;
		}

		AtomicDecrement32(&_session._io_cnt);

		if (recv_err == WSAECONNRESET)
			return;

		OnError(dfLOG_LEVEL_SYSTEM, recv_err, L"WSARecv Fail");
	}
	return;
}

void NetClient::RecvProc(DWORD transferred)
{
	NetSerializeBuffer* packet; 	//Packet�� �ܺο��� �ٽ� ����ϴ� ��찡 �߻��� �� �ִ�. -> �̿� ���� ����� �ʿ��ϴ�. (LAN Server������ ����)
	int32 data_size;
	int num = 0;

	//ReSize ����� �߰��ϱ�
	_session._recv_buf->MoveWritePos(transferred);

	while (true)
	{
		//����͸� ��
		data_size = _session._recv_buf->GetDataSize();

		//��� ũ�� Ȯ��
		if (data_size < sizeof(NetHeader))
		{
			break;
		}

		//len ���� ������ ����
		int packet_len = reinterpret_cast<NetHeader*>(_session._recv_buf->GetReadPos())->_len;

		if (data_size < sizeof(NetHeader) + packet_len)
		{
			break;
		}

		packet = _session._recv_buf->GetNetSerializeBuffer(packet_len);

		if (GetPacket(packet) == false)
		{
			DisconectInLib();
			OnError(dfLOG_LEVEL_SYSTEM, df_HEADER_CHECKSUM_ERROR, L"NetHeader CheckSum ERROR");
			return;
		}

		//packet�� ���� Ref ������ OnRecv���� ó���� �̷������.
		OnRecvMsg(packet);

		//RecvPacket Buffer�� � �Ѱ��ΰ��� �����ؾ� �Ѵ�.
		if (_session._recv_buf->MoveIdxAndPos(packet_len + sizeof(NetHeader)) == false)
		{
			//������ recv_buf�� ���� ������ �Ҵ����� �� �Ҵ��� ���������� �������� �ʰ� ��� ����ϴ� ������ �߻��ߴ�.
			NetRecvPacket* temp_recv_packet = ALLOC_NET_RECV_PACKET();
			temp_recv_packet->CopyRemainData(_session._recv_buf);
			FREE_NET_RECV_PACKET(_session._recv_buf);
			_session._recv_buf = temp_recv_packet;
		}
		num++;
	}

	if (_session._recv_buf->Used())
	{
		NetRecvPacket* temp_recv_packet = ALLOC_NET_RECV_PACKET();
		temp_recv_packet->CopyRemainData(_session._recv_buf);
		FREE_NET_RECV_PACKET(_session._recv_buf);
		_session._recv_buf = temp_recv_packet;
	}

	InterlockedAdd(reinterpret_cast<LONG*>(&_recv_msg_tps), num);
	RecvPost();

	return;
}

void NetClient::ReleasePost()
{
	int32 err_code;

	if (InterlockedCompareExchange(reinterpret_cast<long*>(&_session._io_cnt), RELEASE_MASK, 0) != 0)
		return;

	memset(&_session._recv_overlapped, 0, sizeof(WSAOVERLAPPED));
	_session._recv_overlapped.type = OverlappedEventType::RELEASE_OVERLAPPED;

	if (PostQueuedCompletionStatus(_iocp_core._iocp_handle, 0, 0, reinterpret_cast<OVERLAPPED*>(&_session._recv_overlapped)) == false)
	{
		err_code = WSAGetLastError();
		OnError(dfLOG_LEVEL_SYSTEM, err_code, L"Release Post PQCS Fail");
		__debugbreak();
	}
}

void NetClient::ReleaseProc()
{
	OnRelease();
	_session.SessionLanClear();
}

void NetClient::SendPostOnly()
{
	if (GetSession() == false)
		return;

	if (_session._send_buf.Size() > 0)
		SendPost();

	if (AtomicDecrement32(&_session._io_cnt) == 0)
	{
		ReleasePost();
	}

	return;
}

void NetClient::DisconectInLib()
{
	do
	{
		if (InterlockedExchange8(&_session._disconnect_flag, 1) == 1)
			break;

		::CancelIoEx((HANDLE)_session._socket, nullptr);

	} while (false);
}

bool NetClient::GetSession()
{
	uint32 io_cnt = AtomicIncrement32(&_session._io_cnt);

	if ((io_cnt & RELEASE_MASK) != 0)
	{
		if (AtomicDecrement32(&_session._io_cnt) == 0)
		{
			ReleasePost();
		}

		return false;
	}
	return true;
}
