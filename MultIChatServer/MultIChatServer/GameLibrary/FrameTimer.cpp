#include "pch.h"
#include "FrameTimer.h"
#include "NetService.h"
unsigned FrameTimer::FrameTimerThread(void* service)
{
	reinterpret_cast<NetService*>(service)->FrameTimerThreadLoop();
	return 0;
}
