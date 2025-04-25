#include "pch.h"
#include "LibGlobal.h"
#include "ThreadManager.h"
#include "LogUtils.h"
#include "PerformanceMonitor.h"
#include "Memory.h"
#include "SocketUtils.h"


//ThreadManager* g_thread_manager = nullptr;
LogUtils* g_logutils = nullptr;
PDH* g_pdh = nullptr;
class LibGlobal
{
public:
	LibGlobal()
	{
		_wsetlocale(LC_ALL, L"korean");
		timeBeginPeriod(1);

		//g_thread_manager = new ThreadManager();
		g_logutils = new LogUtils();
		g_pdh = new PDH();
		SocketUtils::WSAInit();
	}

	~LibGlobal()
	{
		//delete g_thread_manager;
		delete g_logutils;
		delete g_pdh;
		SocketUtils::WSAClear();
	}

} g_lib_global;

