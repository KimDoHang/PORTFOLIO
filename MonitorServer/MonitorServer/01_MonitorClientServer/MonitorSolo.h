#pragma once
#include "SoloInstance.h"
#include "NetSerializeBuffer.h"
#include "NetMonitor.h"
#include "NetSendPacket.h"
#include "Values.h"

class MonitorSolo : public SoloInstance
{
public:

	MonitorSolo(NetService* service, uint16 logic_id) : SoloInstance(service, logic_id)
	{
	}


	virtual void OnEnter(uint64 session_id, SessionInstance* player) override;
	virtual void OnLeave(uint64 session_id, SessionInstance* player) override;
	virtual void OnRelease(uint64 session_id, SessionInstance* player) override;
	virtual bool OnRecvMsg(uint64 session_id, SessionInstance* player, NetSerializeBuffer* msg) override;


private:
	void UnMarshalMonitorToolReqLogin(uint64 session_id, NetSerializeBuffer* msg);
	void HandlelMonitorToolReqLogin(uint64 session_id, char* login_key);
	void MarshalMonitorToolResLogin(uint64 session_id, uint16 type, uint8 status);
	void MarshalMonitorToolResLoginDisconnect(uint64 session_id, uint16 type, uint8 status);
};

