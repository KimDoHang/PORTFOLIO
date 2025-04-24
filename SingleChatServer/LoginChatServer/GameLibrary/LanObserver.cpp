#include "pch.h"
#include "LanObserver.h"
#include "Lock.h"
#include "NetService.h"
#include "LanService.h"
#include "ThreadManager.h"

unsigned int LanObserver::LanObserverThread(void* service_ptr)
{
    LanService* service = reinterpret_cast<LanService*>(service_ptr);
    service->ObserverThreadLoop();
    return 0;
}

void LanObserver::ObserverInit(int32 monitor_time)
{
    _monitor_time = monitor_time;
    _accept_total = 0;
    _accept_tps = 0;
    _recv_msg_tps = 0;
    _send_msg_tps = 0;
    _send_msg_avg = 0;
    _recv_msg_avg = 0;
    _out_accept_tps = 0;
    _out_send_msg_tps = 0;
    _out_recv_msg_tps = 0;
    _tick = 0;
    _out_tick = 0;
}

void LanObserver::UpdateLibData()
{
    _tick++;

    _out_accept_tps = AtomicExchange32ToZero(&_accept_tps);
    _out_send_msg_tps = AtomicExchange32ToZero(&_send_msg_tps);
    _out_recv_msg_tps = AtomicExchange32ToZero(&_recv_msg_tps);

    _out_tick = _tick;
    _send_msg_avg += _out_send_msg_tps;
    _recv_msg_avg += _out_recv_msg_tps;
}

