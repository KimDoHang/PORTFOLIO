#pragma once
#include "NetSerializeBuffer.h"
#include "NetClient.h"
#include <format>


#pragma warning(disable : 26495)

class MonitorSession : public NetClient
{
public:
	static const int MAX_GRID_SIZE = 128;
	static const int MIN_GRID_SIZE = 16;

	~MonitorSession()
	{
		DeleteObject(_tile_brush);
		DeleteObject(_grid_pen);
	}

public:
	virtual void OnConnect();
	virtual void OnRelease();
	virtual void OnRecvMsg(NetSerializeBuffer* msg);
	virtual void OnError(const int8 log_level, int32 err_code, const WCHAR* cause);
	virtual void OnExit();

public:
	//IO ���� �Լ���
	void ConnectStart(HWND hWnd);

	void MarshalLoginMsg(uint16 type);
	void MarshalHeartBeatMsg(uint16 type);

	void UnmarshalResLogin(NetSerializeBuffer* buffer);
	void UnmarshalResMonitorInfo(NetSerializeBuffer* buffer);

	void HandleReqLogin(USHORT size_x, USHORT size_y);

	HWND GetMonitorGUIHWND() { return _hWnd; }

public:
	//GUI Function
	void RenderGrid(HDC hdc);
	void RenderObstacle(HDC hdc);
	int GetMaxBlockSize() { return block_size[MAX_BLCOK_SIZE - 1]; }
	void FixGridSize(HWND hWnd, SHORT mouseMove);
	Pos  DrawingRect();

	void WMInit();
	void WMPaint();

private:
	uint8 _server_no;
	time_t _cur_time;
private:
	BOOL _net_connect = false;
	BOOL _net_draw_info = false;
	UINT_PTR _timer_id;
	int32 GRID_WIDTH;
	int32 GRID_HEIGHT;

private:
	int32 _congestion_level1;
	int32 _congestion_level2;
	int32 _congestion_level3;
	int32 _congestion_level4;

private:
	int block_size[4] = { 16,32,64,128 };

	//Ÿ�� ���� �� ũ��
	int _curBlockSIze = 0;
	int GRID_SIZE = 16;
private:
	//Ÿ�� �Ӽ��� ������ 2���� Ÿ�� 
	vector<vector<USHORT>> _tiles;
	//�׸��⿡ ���Ǵ� �귯�� �� ��
	HBRUSH _tile_brush;
	HBRUSH _level1_brush;
	HBRUSH _level2_brush;
	HBRUSH _level3_brush;
	HBRUSH _level4_brush;
	HBRUSH _level5_brush;

	HBRUSH _look_brush;
	HBRUSH _line_brush;

	HPEN _grid_pen;
	HPEN _dir_pen;

	Pos _startRenderPos{ 0,0 };
	Pos _renderPos{ 0,0 };
	Pos _bitmapPos{ 0,0 };
	bool _showData = true;
private:
	HDC _hdcBack;
	HBITMAP _bmpBack;
	RECT _rect;
	HWND _hWnd;
	PAINTSTRUCT _ps;
private:
	char _login_key[df_LOGIN_KEY_SIZE];
	//Ÿ�� Ÿ�� �� ��ֹ� �׸��⿡ ���
	//Erase Ÿ�Կ� ���� ����� ��� Ȥ�� ��ֹ� �׸��� ���
};

#pragma warning(default : 26495)
