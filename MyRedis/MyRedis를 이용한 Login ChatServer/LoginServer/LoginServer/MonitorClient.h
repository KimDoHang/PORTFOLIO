#pragma once
#include "LanClient.h"
#include "LoginProtocol.h"

class MonitorClient : public LanClient
{
public:

	void SendLoginPacket(uint16 type, int16 server_no, uint8 hardware_flag);

	void SendMontiorPacket(uint16 type, uint8 data_type, int32 data_val, int32 time_stamp);

	void UnMarshalMonitorResLogin(LanSerializeBuffer* packet)
	{
		uint8 status;
		*packet >> status;

		if (status == 1)
		{
			_connect_flag = true;
		}
		else
		{
			OnError(dfLOG_LEVEL_SYSTEM, 0, L"Login Server Monitor Login Fail");
			Disconnect();
		}
	}

public:
	__forceinline bool GetConnectFlag() { return _connect_flag; }
public:
	virtual void OnConnect();
	virtual void OnRelease();
	virtual void OnRecvMsg(LanSerializeBuffer* packet);
	virtual void OnError(const int8 log_level, int32 err_code, const WCHAR* cause);
	virtual void OnExit();

private:
	bool _connect_flag = false;
};

