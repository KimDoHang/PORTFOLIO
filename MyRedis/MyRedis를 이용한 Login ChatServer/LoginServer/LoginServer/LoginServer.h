#pragma once
#include "NetService.h"
#include "NetSendPacket.h"
#include "LoginProtocol.h"
#include "SqlUtils.h"
#include "RedisUtils.h"
#include "MonitorClient.h"
#include "LoginConfig.h"
#pragma warning(disable : 4700)


class LoginServer : public NetService
{
	friend class AuthServer;

public:
	LoginServer();
	~LoginServer();
	void LoginInit(const WCHAR* text_file);

public:
	virtual bool OnAccept(uint64 session_id);
	virtual void OnRelease(uint64 session_id);
	virtual bool OnConnectionRequest(WCHAR* IP, SHORT port);
	virtual void OnError(const int8 log_level, uint64 session_id, int32 err_code, const WCHAR* cause);
	virtual void OnError(const int8 log_level, int32 err_code, const WCHAR* cause);
	virtual bool OnTimeOut(uint64 thread_id);
	virtual void OnMonitor();
	virtual void OnExit();

	void LoginStart();
	void LoginStop();
private:
	AuthServer* _auth_server;
	MonitorClient* _monitor_client;
private:
	//Monitoring data
	alignas(64) int32 _update_tps;
	uint64 _update_avg;
	alignas(64) int32 _login_auth_tps;
	uint64 _login_auth_avg;
};

