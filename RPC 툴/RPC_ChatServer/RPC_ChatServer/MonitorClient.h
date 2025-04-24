#pragma once
#include "LanClient.h"
#include "SingleChat_Packet.h"
#include "ChatValues.h"

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
			OnError(dfLOG_LEVEL_SYSTEM, 0, L"Login Chatting Monitor Login Fail");
			__debugbreak();
		}
	}

	void MarshalReqSectorInfo(uint16 type, uint8 server_no, int32 time_stamp, uint16 sector_infos[][MAX_SECTOR_SIZE_X]);

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

