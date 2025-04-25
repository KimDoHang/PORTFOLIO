#pragma once
#include "LoopInstance.h"

class LanService;
class NetService;
struct ThreadInfo;


class Kicker : public LoopInstance
{
	friend class NetService;
	friend class LanService;

public:
	//Timeout Thread
	Kicker(NetService* service, int32 frame_tick) : _service(service), LoopInstance(frame_tick) {}
	virtual ~Kicker() {}

	virtual void Loop() override;


private:
	NetService* _service;
};

//모니터링 지우고 
//PDH, Profiler, 