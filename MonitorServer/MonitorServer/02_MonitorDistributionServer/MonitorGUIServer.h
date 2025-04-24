#pragma once
#include "NetService.h"
#include "NetObserver.h"

class MonitorManager;
class GUISolo;


struct MonitorGUI
{
	uint64 session_id = UINT16_MAX;
	uint8 server_no = 0;
	bool login_flag = false;
};

class MonitorGUIServer : public NetService
{
	friend GUISolo;

public:

	void MonitrGUIInit(MonitorManager* manager);
	virtual bool OnAccept(uint64 session_id);
	virtual void OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause);
	virtual void OnError(const int8 log_level, int32 err_code, const WCHAR* cause);
	virtual bool OnConnectionRequest(WCHAR* IP, SHORT port);
	virtual bool OnTimeOut(uint64 thread_id);
	virtual void OnMonitor();
	virtual void OnExit();

public:
	void SendAllMonitorInfoMsg(uint16 type, uint8 server_no, int32 time_stamp, uint16 sector_infos[][MAX_SECTOR_SIZE_X]);
	char* GetLoginKey();
	
	void PrintMonitor()
	{
		_observer->UpdateLibData();
		PrintLibraryConsole();
		printf("Monitor GUI Client [CUR: %d MAX:%d]\n", _cur_monitor_gui_num, _max_monitor_gui_num);
	}

private:
	MonitorManager* _manager;
	GUISolo* _gui_solo;
	MonitorGUI* _monitor_gui_arr;
	int32 _max_monitor_gui_num;
	int32 _cur_monitor_gui_num;
	int32 _arr_num;
};

