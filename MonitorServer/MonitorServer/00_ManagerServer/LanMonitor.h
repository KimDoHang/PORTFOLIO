#pragma once
#include "LanService.h"
#include "ServerProtocol.h"
#include "Values.h"
#include "LanObserver.h"
class MonitorManager;

struct ConnectServerNo
{
	uint16 _server_no;
	uint8 _hardware_send;
};

class LanMonitor : public LanService
{
public:

	void LanMonitorInit(MonitorManager* manager);

	virtual bool OnAccept(uint64 session_id);
	virtual void OnRelease(uint64 session_id);
	virtual void OnRecvMsg(uint64 session_id, LanSerializeBuffer* msg);
	virtual void OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause);
	virtual void OnError(const int8 log_level, int32 err_code, const WCHAR* cause);
	virtual bool OnConnectionRequest(WCHAR* IP, SHORT port);
	virtual bool OnTimeOut(uint64 session_id);
	virtual void OnMonitor();
	virtual void OnExit();

	void PrintMonitor()
	{
		_observer.UpdateLibData();

		PrintLibraryConsole();
		printf("Connect Lan Server [CUR: %d MAX:%d]\n", _cur_monitor_server_num, _max_monitor_server_num);
	}

private:
	void UnMarshalMonitorServerReqLogin(uint64 session_id, LanSerializeBuffer* msg);
	void HandleLoginServerReqLogin(uint64 session_id, uint8 hardware_flag);
	void HandlelChatServerReqLogin(uint64 session_id, uint8 hardware_flag);
	void HandlelGameServerReqLogin(uint64 session_id, uint8 hardware_flag);

	void UnMarshalMonitorServerUpdate(uint64 session_id, LanSerializeBuffer* msg);
	void HandlelMonitorServerUpdate(uint64 session_id, uint8 data_type, int32 data_val, int32 time_stamp);

	void UnMashalMonitorGUIServerUpdate(uint64 session_id, LanSerializeBuffer* msg);
	void HandleMonitorGUIServerUpdate(uint64 session_id, uint8 server_no, int32 time_stamp, uint16 sector_infos[][MAX_SECTOR_SIZE_X]);


	void MarshalMonitorLoginRes(uint64 session_id, uint16 type, uint8 status);

	uint8 CheckType(uint64 session_id, uint8 data_type, int32 data_val, int32 time_stamp);


private:
	MonitorManager* _manager;
	ConnectServerNo* _monitor_servers;
	int32 _max_monitor_server_num;
	int32 _cur_monitor_server_num;
	int32 _arr_num;
};

