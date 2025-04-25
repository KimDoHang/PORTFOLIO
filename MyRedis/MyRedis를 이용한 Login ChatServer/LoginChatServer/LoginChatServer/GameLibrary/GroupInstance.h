#pragma once
#include "LockFreeQueueStatic.h"
#include "Job.h"
#include "LogicInstance.h"
#include "LoopInstance.h"

class SessionInstance;
class NetSerializeBuffer;
class NetSession;


//Dedicated SessionInstance는 반드시 모든 Session과 Job이 처리되는 것을 보장한 후 제거되어야 한다.
//동적 생성이 가능한 형식으로 구성하였다.
class GroupInstance : public LogicInstance, public LoopInstance
{
	friend class NetService;

public:

	GroupInstance(NetService* service, uint16 logic_id, uint64 frame_tick) : LogicInstance(LogicType::GroupType, service, logic_id), LoopInstance(frame_tick)
	{
	}

	virtual ~GroupInstance();

public:
	// 상속 함수 제공
	virtual void OnEnter(uint64 session_id, SessionInstance* player) abstract;
	virtual void OnLeave(uint64 session_id, SessionInstance* player) abstract;
	virtual void OnRelease(uint64 session_id, SessionInstance* player) abstract;
	virtual bool OnRecvMsg(uint64 session_id, SessionInstance* player, NetSerializeBuffer* msg) abstract;
	virtual void OnFrame() abstract;

protected:
	//Session 관리 Hash
	unordered_map<uint64, NetSession*> _session_player_hash;
	//Session 등록, 제거 Job 처리 Queue
	LockFreeQueueStatic<Job*> _job_queue;

public:
	virtual void RegisterAccept(uint64 session_id) override;
	virtual void RegisterEnter(uint64 session_id, LogicType prev_stream_type) override;
	virtual void RegisterLeave(uint64 session_id, uint16 logic_id) override;
	virtual void RegisterRelease(NetSession* session) override;
	virtual void RegisterExit() override;
private:
	virtual void RecvMsg(NetSession* session, NetSerializeBuffer* msg) override;
	virtual void Loop() override;
	bool JobUpdate();
	void MsgUpdate();
	void FrameUpdate();
	//Work In IOCP

};

