#pragma once

class NetService;

class LoopInstance
{
public:
	friend class NetService;

#pragma warning (disable : 26495)

	LoopInstance(uint64 frame_tick) : _frame_tick(frame_tick), _cur_console_time(0), _past_console_time(0)
	{
		_loop_overlapped.type = OverlappedEventType::LOOP_OVERLAPPED;
		_loop_overlapped.instance = this;
	}

#pragma warning (default : 26495)

	virtual ~LoopInstance() {} 

	virtual void Loop() abstract;
	void SetLastTick() { _last_tick = timeGetTime(); }
	void SetLastTick(int32 last_tick) { _last_tick = last_tick; }
	uint64 GetConsoleTime();
	__forceinline OVERLAPPED* GetLoopOverlapped() { return &_loop_overlapped; }
protected:
	uint64 _frame_tick;
	uint64 _last_tick;
	OverlappedEvent _loop_overlapped;
	uint64 _cur_console_time;
	uint64 _past_console_time;
};

