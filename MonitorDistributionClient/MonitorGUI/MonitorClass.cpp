#include "pch.h"
#include "MonitorClass.h"
#include "TextParser.h"
#include "LogUtils.h"
#include "MonitorProtocol.h"
#include <format>


void MonitorSession::OnConnect()
{
	int32 err_code = 0;

	MarshalLoginMsg(en_PACKET_CS_MONITOR_DISTRIBUTION_REQ_LOGIN);

	//Server에서는 TimeOut Thread가 돌기 때문에 HeartBeat를 지속적으로 보내주어야 한다.
	//이때 OnConnect 이후부터 바로 시간초가 들어가기 때문에 바로 타이머를 설정하고 전달한다.
	//연결이 실패한 경우에는 어차피 해당 msg가 버려지므로 문제가 되지 않는다.
	_timer_id = SetTimer(_hWnd, HEART_BEAT_TIMER, 3000, NULL);

	if (_timer_id == 0)
	{
		err_code = ::WSAGetLastError();

		WCHAR* log_buff = g_logutils->GetLogBuff(L"Create Window Timer Fail [ErrCode:%d]", err_code);
		g_logutils->Log(NET_LOG_DIR, df_MONITOR_LOG_FILE, log_buff);
		Disconnect();
	}
}

void MonitorSession::OnRelease()
{

	//Recv에서만 호출되고 삭제될 때 호출되므로 언제나 싱글 스레드에서 동작한다.
	//또한 반드시 완벽하게 동기화가 될 필요는 없다. (단순히 해당 msg를 표시하지 못할뿐 중요X)

	//확인 창 및 메시지 박스의 I아이콘을 띄워주는 매크로이다.
	MessageBox(NULL, L"Monitor GUI가 종료됩니다.", L"알림", MB_OK | MB_ICONINFORMATION);
	Sleep(2000);

	_net_connect = false;
	_net_draw_info = false;
	KillTimer(_hWnd, _timer_id);

	//WM_QUIT을 윈도우로 던져 준다. 이때 GetMessage에서는 해당 WM_QUIT을 받게 되면 0을 반환해준다.
	PostMessage(_hWnd, WM_QUIT, 0, 0);
	g_logutils->Log(NET_LOG_DIR, df_MONITOR_LOG_FILE, L"MonitorGUI OFF");
}

void MonitorSession::OnRecvMsg(NetSerializeBuffer* msg)
{
	WORD type = UINT16_MAX;

	try
	{
		*msg >> type;

		switch (type)
		{
		case en_PACKET_SC_MONITOR_DISTRIBUTION_RES_LOGIN:
			UnmarshalResLogin(msg);
			break;
		case en_PACKET_SC_MONITOR_DISTRIBUTION_SECTOR_INFO:
			UnmarshalResMonitorInfo(msg);
			break;
		default:
			g_logutils->Log(NET_LOG_DIR, df_MONITOR_LOG_FILE, L"Recv MSG Type Error");
			Disconnect();
			break;
		}
	}
	catch (const NetMsgException& net_msg_exception)
	{
		OnError(dfLOG_LEVEL_SYSTEM, type, net_msg_exception.whatW());
		Disconnect();
	}

}

void MonitorSession::OnError(const int8 log_level, int32 err_code, const WCHAR* cause)
{
	WCHAR log_buff[200];
	if (g_logutils->GetLogLevel() <= log_level)
	{
		wsprintfW(log_buff, L"%s [ErrCode:%d]", cause, err_code);
		g_logutils->Log(NET_LOG_DIR, df_MONITOR_LOG_FILE, log_buff);
	}
}

void MonitorSession::OnExit()
{

}


void MonitorSession::MarshalLoginMsg(uint16 type)
{
	NetSerializeBuffer* msg = ALLOC_NET_PACKET();
	*msg << type << _server_no;
	msg->PutData(_login_key, df_LOGIN_KEY_SIZE);
	SendPacket(msg);
	FREE_NET_SEND_PACKET(msg);
}

void MonitorSession::MarshalHeartBeatMsg(uint16 type)
{
	NetSerializeBuffer* msg = ALLOC_NET_PACKET();
	*msg << type;
	SendPacket(msg);
	FREE_NET_SEND_PACKET(msg);
}


void MonitorSession::UnmarshalResLogin(NetSerializeBuffer* buffer)
{
	USHORT size_x;
	USHORT size_y;
	BYTE status;

	*buffer >> size_x >> size_y >> status;

	if (status == false)
	{
		g_logutils->Log(NET_LOG_DIR, df_MONITOR_LOG_FILE, L"Req Login Fail (Status False)");
		Disconnect();
		return;
	}

	HandleReqLogin(size_x, size_y);
}
void MonitorSession::UnmarshalResMonitorInfo(NetSerializeBuffer* buffer)
{
	int32 cur_time;
	if (_net_connect == false)
	{
		g_logutils->Log(NET_LOG_DIR, df_MONITOR_LOG_FILE, L"Send Monitor Info Before ReqLogin");
		Disconnect();
		return;
	}

	*buffer >> cur_time;


	USHORT player_num;

	for (int y = 0; y < GRID_HEIGHT; y++)
	{
		for (int x = 0; x < GRID_WIDTH; x++)
		{
			*buffer >> player_num;
			_tiles[y][x] = player_num;
		}
	}

	if (_net_draw_info == false)
	{
		_net_draw_info = true;
	}
	

	_cur_time = cur_time;

	InvalidateRect(_hWnd, NULL, FALSE);
}

void MonitorSession::HandleReqLogin(USHORT size_x, USHORT size_y)
{
	GRID_HEIGHT = size_y;
	GRID_WIDTH = size_x;

	_tiles.resize(GRID_HEIGHT, vector<USHORT>(GRID_WIDTH));

	_grid_pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
	_dir_pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	_tile_brush = CreateSolidBrush(RGB(100, 100, 100));
	_look_brush = CreateSolidBrush(RGB(255, 153, 102));
	_line_brush = CreateSolidBrush(RGB(0, 0, 0));

	_level1_brush = CreateSolidBrush(RGB(255, 255, 255));
	_level2_brush = CreateSolidBrush(RGB(108, 190, 69));
	_level3_brush = CreateSolidBrush(RGB(252, 204, 10));
	_level4_brush = CreateSolidBrush(RGB(255, 99, 25));
	_level5_brush = CreateSolidBrush(RGB(255, 0, 0));

	WMInit();

	_net_connect = true;
}

void MonitorSession::ConnectStart(HWND hWnd)
{

	WCHAR ip[20];
	WCHAR log_level[20];

	int32 port;
	int32 concurrent_thread_num;
	int32 max_thread_num;
	int32 server_no;
	int32 packet_code;
	int32 packet_key;

	TextParser parser;

	bool textfile_load_ret = parser.LoadFile(df_MONITOR_FILE);

	if (textfile_load_ret == false)
		__debugbreak();

	parser.GetString(L"CONNECT_IP", ip);
	parser.GetValue(L"CONNECT_PORT", &port);
	parser.GetValue(L"IOCP_WORKER_THREAD", &max_thread_num);
	parser.GetValue(L"IOCP_ACTIVE_THREAD", &concurrent_thread_num);
	parser.GetValue(L"SERVER_NO", &server_no);
	parser.GetValue(L"PACKET_CODE", &packet_code);
	parser.GetValue(L"PACKET_KEY", &packet_key);
	parser.GetValue(L"CONGESTION_LEVEL_1", reinterpret_cast<int*>(&_congestion_level1));
	parser.GetValue(L"CONGESTION_LEVEL_2", reinterpret_cast<int*>(&_congestion_level2));
	parser.GetValue(L"CONGESTION_LEVEL_3", reinterpret_cast<int*>(&_congestion_level3));
	parser.GetValue(L"CONGESTION_LEVEL_4", reinterpret_cast<int*>(&_congestion_level4));

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

	_hWnd = hWnd;
	_server_no = server_no;
	memcpy(_login_key, "ajfw@!cv980dSZ[fje#@fdj123948djf", df_LOGIN_KEY_SIZE);

	Init(ip, port, concurrent_thread_num, max_thread_num, packet_code, packet_key, true, false);
}

void MonitorSession::RenderGrid(HDC hdc)
{
	int iX = 0;
	int iY = 0;

	HPEN hOldPen = (HPEN)SelectObject(hdc, _grid_pen);

	for (int wCnt = 0; wCnt <= GRID_WIDTH; wCnt++)
	{
		MoveToEx(hdc, iX, 0, NULL);
		LineTo(hdc, iX, GRID_HEIGHT * GRID_SIZE);
		iX += GRID_SIZE;
	}

	for (int hCnt = 0; hCnt <= GRID_HEIGHT; hCnt++)
	{
		MoveToEx(hdc, 0, iY, NULL);
		LineTo(hdc, GRID_SIZE * GRID_WIDTH, iY);
		iY += GRID_SIZE;
	}

	SelectObject(hdc, hOldPen);
}

void MonitorSession::RenderObstacle(HDC hdc)
{
	//Render Obstacle 찾아서 그리면된다 ㅎㅎ

	HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, _tile_brush);
	HPEN hOldPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));

	//위치를 연산해주어야 한다. 
	//특정 지점의 시작 지점을 찾고 
	int iX = 0;
	int iY = 0;

	for (int hCnt = 0; hCnt < GRID_HEIGHT; hCnt++)
	{
		for (int wCnt = 0; wCnt < GRID_WIDTH; wCnt++)
		{
			USHORT congestion = _tiles[hCnt][wCnt];
			iX = GRID_SIZE * wCnt;
			iY = GRID_SIZE * hCnt;

			if (congestion < _congestion_level1)
			{
				SelectObject(hdc, _level1_brush);
				Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);
			}
			else if (congestion < _congestion_level2)
			{
				SelectObject(hdc, _level2_brush);
				Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);
			}
			else if (congestion < _congestion_level3)
			{
				SelectObject(hdc, _level3_brush);
				Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);
			}
			else if (congestion < _congestion_level4)
			{
				SelectObject(hdc, _level4_brush);
				Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);
			}
			else
			{
				SelectObject(hdc, _level5_brush);
				Rectangle(hdc, iX, iY, iX + GRID_SIZE + 2, iY + GRID_SIZE + 2);
			}


			if (_showData != false && _curBlockSIze == BLOCK_SIZE::MAX_BLCOK_SIZE - 1)
			{
				wstring str2 = ::format(L"PlayerNum {0}", _tiles[hCnt][wCnt]);
				wstring str1 = ::format(L"Pos: ({0},{1})", wCnt, hCnt);
				TextOutW(hdc, iX, iY, str1.c_str(), (int)str1.size());
				TextOutW(hdc, iX, iY + 20, str2.c_str(), (int)str2.size());
			}

		}
	}

	SelectObject(hdc, hOldBrush);
	SelectObject(hdc, hOldPen);
}


void MonitorSession::FixGridSize(HWND hWnd, SHORT mouseMove)
{
	POINT mousePos;
	::GetCursorPos(&mousePos);
	ScreenToClient(hWnd, &mousePos);

	int prevBlockSize = _curBlockSIze;
	if (mouseMove <= 0)
	{
		_curBlockSIze = max(BLOCK_SIZE::BLOCK16, _curBlockSIze - 1);
	}
	else if (mouseMove > 0)
	{
		_curBlockSIze = min(BLOCK_SIZE::BLOCK128, _curBlockSIze + 1);
	}

	if (prevBlockSize == BLOCK_SIZE::BLOCK16 && _curBlockSIze == BLOCK_SIZE::BLOCK32)
	{
		_bitmapPos.x = mousePos.x;
		_bitmapPos.y = mousePos.y;

	}

	GRID_SIZE = block_size[_curBlockSIze];
	InvalidateRect(hWnd, NULL, false);
}

Pos MonitorSession::DrawingRect()
{
	_renderPos.x = (block_size[_curBlockSIze] / block_size[0]) * _bitmapPos.x - _bitmapPos.x;
	_renderPos.y = (block_size[_curBlockSIze] / block_size[0]) * _bitmapPos.y - _bitmapPos.y;

	int x = _renderPos.x / GRID_SIZE;
	int y = _renderPos.y / GRID_SIZE;

	//	return Pos{ _renderPos.x - x * GRID_SIZE,  _renderPos.y - y * GRID_SIZE };
	return _renderPos;
}

void MonitorSession::WMInit()
{
	HDC hdc = GetDC(_hWnd);
	GetClientRect(_hWnd, &_rect);
	_hdcBack = ::CreateCompatibleDC(hdc);
	_bmpBack = ::CreateCompatibleBitmap(hdc, 128 * GRID_WIDTH, 128 * GRID_HEIGHT);
	HBITMAP prevBitmap = (HBITMAP)::SelectObject(_hdcBack, _bmpBack);
	DeleteObject(prevBitmap);
}

void MonitorSession::WMPaint()
{
	if (_net_draw_info == false || _net_connect == false)
	{
		BeginPaint(_hWnd, &_ps);
		EndPaint(_hWnd, &_ps);
		return;
	}

	HDC hdc = BeginPaint(_hWnd, &_ps);
	Pos pos = DrawingRect();

	
	tm curLocalTime;
	localtime_s(&curLocalTime, &_cur_time);

	::PatBlt(_hdcBack, pos.x, pos.y, 16 * GRID_WIDTH + 20, 16 * GRID_HEIGHT + 20, WHITENESS);
	RenderObstacle(_hdcBack);
	RenderGrid(_hdcBack);
	
	WCHAR print_data[128];

	switch (_server_no)
	{
	case ChatServerNo:
		wsprintf(print_data, L"(Chatserver [%d/%02d/%02d %02d:%02d:%02d])", curLocalTime.tm_year + 1900, curLocalTime.tm_mon + 1, curLocalTime.tm_mday, curLocalTime.tm_hour, curLocalTime.tm_min, curLocalTime.tm_sec);
		break;
	case GameServerNo:
		wsprintf(print_data, L"(GameServer [%d/%02d/%02d %02d:%02d:%02d])", curLocalTime.tm_year + 1900, curLocalTime.tm_mon + 1, curLocalTime.tm_mday, curLocalTime.tm_hour, curLocalTime.tm_min, curLocalTime.tm_sec);
		break;
	default:
		Disconnect();
		return;
	}

	TextOutW(_hdcBack, 0, 0, print_data, (int32)(wcslen(print_data)));


	::BitBlt(hdc, 0, 0, 16 * GRID_WIDTH, 16 * GRID_HEIGHT, _hdcBack, pos.x, pos.y, SRCCOPY);
	EndPaint(_hWnd, &_ps);
}

