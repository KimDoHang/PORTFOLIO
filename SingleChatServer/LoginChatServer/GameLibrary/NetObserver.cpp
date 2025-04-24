#include "pch.h"
#include "NetObserver.h"


void NetObserver::Loop()
{

	UpdateLibData();
	bool ret = _service->ObserverThreadLoopIOCP();

	if (ret == false)
	{
		_service->ExitLoopInstance();
		return;
	}
	_service->RegisterLoop(static_cast<LoopInstance*>(this));
}

void NetObserver::UpdateLibData()
{
	_tick++;

	_out_accept_tps = AtomicExchange32ToZero(&_accept_tps);
	_out_send_msg_tps = AtomicExchange32ToZero(&_send_msg_tps);
	_out_recv_msg_tps = AtomicExchange32ToZero(&_recv_msg_tps);

	_out_tick = _tick;
	_send_msg_avg += _out_send_msg_tps;
	_recv_msg_avg += _out_recv_msg_tps;
}

