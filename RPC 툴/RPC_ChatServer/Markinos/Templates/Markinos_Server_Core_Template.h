#pragma once
#include "NetService.h"
#include "NetSendPacket.h"


class {{ server_name }} : public NetService
{

public:
	virtual bool OnAccept(uint64 session_id);
	virtual bool OnConnectionRequest(WCHAR* IP, SHORT port);
	virtual void OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause) override;
	virtual void OnError(const int8 log_level, int32 err_code, const WCHAR* cause) override;
	virtual bool OnTimeOut(uint64 session_id);
	virtual void OnMonitor() override;
	virtual void OnExit() override;

};