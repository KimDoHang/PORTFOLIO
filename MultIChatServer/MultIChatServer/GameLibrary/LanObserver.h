#pragma once
#include "LoopInstance.h"

struct ThreadInfo;

class LanObserver
{
	friend class NetService;
	friend class LanService;

	static unsigned int LanObserverThread(void* service_ptr);

public:
	//Thread Control
	void ObserverInit(int32 monitor_time);
	void UpdateLibData();

	int32 GetAcceptMsgTPS() { return _out_accept_tps; }
	int32 GetSendMsgTPS() { return _out_send_msg_tps; }
	int32 GetRecvMsgTPS() { return _out_recv_msg_tps; }

	uint64 GetAcceptTotal() { return _accept_total; }
	int32 GetAcceptAvg() { return (int32)(_accept_total / _out_tick); }
	int32 GetSendMsgAvg() { return (int32)(_send_msg_avg / _out_tick); }
	int32 GetRecvMsgAvg() { return (int32)(_send_msg_avg / _out_tick); }
	uint64 GetTick() { return _out_tick; }
private:
	int32 _monitor_time;
	uint64 _accept_total;
	int32 _accept_tps;
	int32 _recv_msg_tps;
	uint64 _recv_msg_avg;
	int32 _send_msg_tps;
	uint64 _send_msg_avg;
	int32 _out_accept_tps;
	int32 _out_send_msg_tps;
	int32 _out_recv_msg_tps;
	uint64 _tick;
	uint64 _out_tick;

};


