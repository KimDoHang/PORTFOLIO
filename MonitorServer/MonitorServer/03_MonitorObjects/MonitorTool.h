#pragma once
struct MonitorTool
{
	MonitorTool()
	{
		login_flag = false;
		session_id = UINT16_MAX;
		InitializeCriticalSection(&_monitor_tool_cs);

	}
	~MonitorTool()
	{
		DeleteCriticalSection(&_monitor_tool_cs);
	}
	uint64 session_id;
	bool login_flag;
	CRITICAL_SECTION _monitor_tool_cs;
};

