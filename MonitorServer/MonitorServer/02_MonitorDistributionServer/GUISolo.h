#pragma once

#include "SoloInstance.h"
#include "NetSerializeBuffer.h"
#include "NetSendPacket.h"
#include "NetMonitor.h"
#include "Values.h"

class GUISolo : public SoloInstance
{
public:
	GUISolo(NetService* service, uint16 logic_id) : SoloInstance(service, logic_id)
	{
	}


	virtual void OnEnter(uint64 session_id, SessionInstance* player) override;
	virtual void OnLeave(uint64 session_id, SessionInstance* player) override;
	virtual void OnRelease(uint64 session_id, SessionInstance* player) override;
	virtual bool OnRecvMsg(uint64 session_id, SessionInstance* player, NetSerializeBuffer* msg) override;

private:
	void UnMarshaMonitorGUILogin(uint64 session_id, NetSerializeBuffer* msg);
	
	void HandleMonitorGUILogin(uint64 session_id, uint8 server_no, char* login_key);
	void HandleMonitorGUIHeartBeatReq(uint64 session_id);

	void MarshalResMonitorLogin(uint64 session_id, uint16 type, uint16 sector_x_size, uint16 sector_y_size, BYTE status);


};

