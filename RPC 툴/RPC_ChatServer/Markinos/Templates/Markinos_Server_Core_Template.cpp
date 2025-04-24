#include "pch.h"
#include "{{ server_name }}.h"
#include "{{ packet_name }}.h"
#include "NetSerializeBuffer.h"
#include "TextParser.h"
#include "LogUtils.h"
#include "PerformanceMonitor.h"
#include "ThreadManager.h"
#include "NetObserver.h"

bool {{ server_name }}::OnAccept(uint64 session_id)
{
	return true;
}


bool {{ server_name }}::OnConnectionRequest(WCHAR* IP, SHORT port)
{
	return true;
}

void {{ server_name }}::OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause)
{
}

void {{ server_name }}::OnError(const int8 log_level, int32 err_code, const WCHAR* cause)
{

}

bool {{ server_name }}::OnTimeOut(uint64 session_id)
{
	Disconnect(session_id);
	return true;
}

void {{ server_name }}::OnMonitor()
{
}

void {{ server_name }}::OnExit()
{
}