#pragma once
#include "LoopInstance.h"
#include "NetService.h"
class NetService;



class NetObserver : public LoopInstance
{
public:
	friend class NetService;


	NetObserver(NetService* service, int32 frame_tick) : _service(service), LoopInstance(frame_tick)
	{

	}
	virtual ~NetObserver() {}	
	virtual void Loop() override;

public:
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
	NetService* _service;
	uint64 _accept_total = 0;
	int32 _accept_tps = 0;
	int32 _recv_msg_tps = 0;
	uint64 _recv_msg_avg = 0;
	int32 _send_msg_tps = 0;
	uint64 _send_msg_avg = 0;
	int32 _out_accept_tps = 0;
	int32 _out_send_msg_tps = 0;
	int32 _out_recv_msg_tps = 0;
	uint64 _tick = 0;
	uint64 _out_tick = 0;
};



