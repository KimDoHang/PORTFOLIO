#include "pch.h"
#include "Kicker.h"
#include "Lock.h"
#include "ThreadManager.h"
#include "NetService.h"
#include "LanService.h"


void Kicker::Loop()
{
	_cur_console_time = timeGetTime();
	bool ret = _service->TimeOutThreadLoop();
	cout << "Kicker LOOP TIME: " << _cur_console_time - _past_console_time << '\n';
	_past_console_time = _cur_console_time;

	if (ret == false)
	{
		_service->ExitLoopInstance();
		return;
	}

	_service->RegisterLoop(static_cast<LoopInstance*>(this));
}
