#include "pch.h"
#include "MonitorManager.h"
#include <conio.h>
#include "Profiler.h"
#include "CrashDump.h"
#include "ThreadManager.h"


CCrashDump dump;
MonitorManager* monitor_manager;

int main()
{
	monitor_manager = new MonitorManager;
	monitor_manager->MonitorInit();
	monitor_manager->MonitorStart();

	while (true)
	{
		Sleep(INFINITE);
	}

}