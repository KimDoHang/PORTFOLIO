#pragma once
#include "LockFreeQueueStatic.h"
#include "Job.h"
#include "LogicInstance.h"

class SessionInstance;
class NetService;
class NetSerializeBuffer;
class NetSession;

class SoloInstance : public LogicInstance
{
	friend NetService;

public:

	SoloInstance(NetService* service, uint16 logic_id) : LogicInstance(LogicType::SoloType, service, logic_id)
	{

	}

	virtual ~SoloInstance();

public:
	virtual void OnEnter(uint64 session_id, SessionInstance* player) abstract;
	virtual void OnLeave(uint64 session_id, SessionInstance* player) abstract;
	virtual void OnRelease(uint64 session_id, SessionInstance* player) abstract;
	virtual bool OnRecvMsg(uint64 session_id, SessionInstance* player, NetSerializeBuffer* msg) abstract;
public:
	virtual void RegisterAccept(uint64 session_id) override;
	virtual void RegisterEnter(uint64 session_id, LogicType prev_stream_type) override;
	virtual void RegisterLeave(uint64 session_id, uint16 logic_id) override;
	virtual void RegisterRelease(NetSession* session) override;
	virtual void RegisterExit() override;

	///<summary> OnAccept에서 반드시 호출 필요

private:
	virtual void RecvMsg(NetSession* session, NetSerializeBuffer* msg) override;
};

