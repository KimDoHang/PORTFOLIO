#pragma once
#include "NetService.h"
#include "ServerProtocol.h"
#include "Values.h"
#include "MonitorTool.h"
#include "NetObserver.h"

//Monitor tool은 각자 동작하게 된다.
//Accept시에 전체 player 수를 비교한다.
//Release시에는 어차피 반환되기 전이고 OnRecvMsg(에서 직렬 처리되므로) 같은 session에 대해서는 직렬적으로 처리가 이루어져도 된다.
//즉 login flag에 대한 변경이 필요없다.
//msg에 대한 send도 마찬가지이다.


class MonitorManager;
class MonitorSolo;

class NetMonitor : public NetService
{
	friend MonitorSolo;

public:

	NetMonitor()
	{
	}

	void NetMonitorInit(MonitorManager* manager);

	virtual bool OnAccept(uint64 session_id);
	virtual void OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause);
	virtual void OnError(const int8 log_level, int32 err_code, const WCHAR* cause);
	virtual bool OnConnectionRequest(WCHAR* IP, SHORT port);
	virtual bool OnTimeOut(uint64 thread_id);
	virtual void OnMonitor();
	virtual void OnExit();
	
	void SendAllUpdateData(uint16 type, uint8 server_no, uint8 data_type, int32 data_val, int32 time_stamp);
	char* GetLoginKey();

	void PrintMonitor()
	{
		_observer->UpdateLibData();
		PrintLibraryConsole();
		printf("Monitor Client [CUR: %d MAX:%d]\n", _cur_tool_num, _max_tool_num);
	}

	
private:
	MonitorManager* _manager;
	MonitorSolo* _monitor_solo;
	MonitorTool* _monitor_tool_client_arr;
	int32 _max_tool_num;
	int32 _cur_tool_num;
	int32 _arr_num;
};

