#pragma once
#include "NetService.h"
#include "ServerProtocol.h"
#include "Values.h"
#include "MonitorTool.h"
#include "NetObserver.h"

//Monitor tool�� ���� �����ϰ� �ȴ�.
//Accept�ÿ� ��ü player ���� ���Ѵ�.
//Release�ÿ��� ������ ��ȯ�Ǳ� ���̰� OnRecvMsg(���� ���� ó���ǹǷ�) ���� session�� ���ؼ��� ���������� ó���� �̷������ �ȴ�.
//�� login flag�� ���� ������ �ʿ����.
//msg�� ���� send�� ���������̴�.


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

