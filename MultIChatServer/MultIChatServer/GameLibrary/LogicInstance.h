#pragma once
#include "LockFreeQueueStatic.h"
#include "Job.h"
class SessionInstance;
class NetSerializeBuffer;
class NetSession;
class NetService;

//Dedicated SessionInstance�� �ݵ�� ��� Session�� Job�� ó���Ǵ� ���� ������ �� ���ŵǾ�� �Ѵ�.
//���� ������ ������ �������� �����Ͽ���.
class LogicInstance
{
	friend class NetService;

public:

	LogicInstance(LogicType stream_type, NetService* service, uint16 logic_id) : _logic_type(stream_type), _logic_id(logic_id), _service(service)
	{
	}

	virtual ~LogicInstance();

public:
	virtual void OnEnter(uint64 session_id, SessionInstance* player) abstract;
	virtual void OnLeave(uint64 session_id, SessionInstance* player) abstract;
	virtual void OnRelease(uint64 session_id, SessionInstance* player) abstract;
	virtual bool OnRecvMsg(uint64 session_id, SessionInstance* player, NetSerializeBuffer* msg) abstract;
public:
	virtual void RegisterAccept(uint64 session_id) abstract;
	virtual void RegisterEnter(uint64 session_id, LogicType prev_stream_type) abstract;
	virtual void RegisterLeave(uint64 session_id, uint16 logic_id) abstract;
	virtual void RegisterRelease(NetSession* session) abstract;
	virtual void RegisterExit() abstract;

public:
	__forceinline LogicType GetLogicType() { return _logic_type; }
	__forceinline uint16 GetLogicArrID() { return _logic_id; }
	void Disconnect(uint64 session_id);

private:
	virtual void RecvMsg(NetSession* session, NetSerializeBuffer* packet) abstract;
protected:
	LogicType _logic_type;
	NetService* _service;
	uint16 _logic_id;
};

