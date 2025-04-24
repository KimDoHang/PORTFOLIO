#pragma once
#include "Job.h"
#include "LockFreeQueueStatic.h"
#include "LoopInstance.h"
struct TimeInfo
{
	TimeInfo(LoopInstance* param_instance, uint64 param_next_frame_tick)
	{
		instance = param_instance;
		next_frame_tick = param_next_frame_tick;
	}

	bool operator<(const TimeInfo& timeinfo) const
	{
		return this->next_frame_tick > timeinfo.next_frame_tick;
	}

	LoopInstance* instance;
	uint64 next_frame_tick;
};

class FrameTimer
{
	friend class NetService;

public:
	static unsigned FrameTimerThread(void* service);

	FrameTimer() : _tick_coefficient(1)
	{

	}

	void SetTickSlower(uint8 num = 2)
	{
		AtomicExchange8(&_tick_coefficient, num);
	}

	void RestoreTick()
	{
		AtomicExchange8(&_tick_coefficient, 1);
	}

	
private:
	int8 _tick_coefficient;
	LockFreeQueueStatic<Job*> _job_queue;
	priority_queue<TimeInfo> _timeinfo_pq;
};

