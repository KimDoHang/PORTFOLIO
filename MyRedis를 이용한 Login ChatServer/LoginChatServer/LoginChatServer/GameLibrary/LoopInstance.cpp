#include "pch.h"
#include "LoopInstance.h"
#include "NetService.h"

uint64 LoopInstance::GetConsoleTime()
{
	_cur_console_time = timeGetTime();

	return _cur_console_time - _past_console_time;
}
