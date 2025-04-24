#include "pch.h"
#include "SoloInstance.h"
#include "NetSession.h"
#include "NetService.h"
#include "SessionInstance.h"

SoloInstance::~SoloInstance()
{
}


//멀티이므로 항상 io_cnt가 1을 물고 있으므로 session_id 직접 전달 괜춘

void SoloInstance::RecvMsg(NetSession* session, NetSerializeBuffer* msg)
{
	OnRecvMsg(session->GetSessionID(), session->GetSessionInstance(), msg);
}

//싱글 -> 멀티 (상대방측이 1을 올린 후 접근), 멀티 -> 멀티 (어차피 RecvProc이므로 굳이 올리지 않더라도 session 제거가 이루어지지 않는다.
//따라서 이전의 상태가 싱글이라면 cnt를 내려주는 작업만을 진행한다. 이떄 해당 작업은 Single Dedi SessionInstance 공간에서 일어난다.
//따라서 OnEnter 호출과 RecvMsg 처리가 동시에 이루어질 수 잇는데 이는 반드시 OnEnter에서 Enter Msg를 마지막에 보내줌으로 써 해결이 가능하다.
//또한 OnEnter까지 cnt를 잡고 있는데 이는 이전에 Session을 반환할 경우 내부적으로 Single에서는 session_cnt를 물지 않기 때문에
//OnEnter와 OnRelease가 동시에 호출되는 것이 가능하다. -> 멀티로 상태가 변경 됬으므로 IOCP Worker에서 곧바로 OnRelease 호출, OnEnter에서도 동일한 session_id에 대한 처리
//이루어진다. 따라서 이를 막기 위해서 OnEnter 이후 session_cnt를 감소 시켰다.
void SoloInstance::RegisterEnter(uint64 session_id, LogicType prev_stream_type)
{
	NetSession* session = _service->GetSessionInLib(session_id);

	session->SetLogicInstance(this);
	OnEnter(session_id, session->GetSessionInstance());

	if (prev_stream_type == LogicType::GroupType)
		_service->ReturnSession(session);
}

//멀티에서는 항상 session_cnt를 물고 있으므로 직접 session에 접근이 가능하다.
void SoloInstance::RegisterLeave(uint64 session_id, uint16 logic_id)
{
	NetSession* session = _service->GetSessionInLib(session_id);

	//OnLeave 및 컨텐츠 정리
	OnLeave(session_id, session->GetSessionInstance());

	//LogicArr로부터 다음 Content Class를 찾은 후 해당 클래스의 Enter 함수 호출
	_service->GetLogicInstance(logic_id)->RegisterEnter(session_id, _logic_type);
}

//기존의 ResetPost, ReleasePost 대신 각 컨텐츠에 맞는 ReleasePost가 호출되어야 한다.
//따라서 io_cnt가 0이되었을 때 각 컨텐츠의 Release가 호출된다.
//이때 Release가 호출되는 것은 반드시 특정 컨텐츠 영역에 포함된 상태이고 재활용이 안되므로 (id 밚롼전이므로)
//session에 대해 직접 접근이 가능하다.
//멀티의 경우 Register에서 바로 ReleaseProc이 호출 가능한 것은 RecvProc을 통해 로직을 처리하고 있는 중에는 삭제가 불가능하다.
//따라서 호출되는 곳은 해당 함수만을 직접 호출하는 곳 밖에 존재하지 않는다. 따라서 바로 처리가 가능하다.
//재귀 Lock이 발생하는 경우는 -> Update에서 처리중 io_cnt를 물지 않고 처리가 이루어지다가 SendPacket등으로 io_cnt를 올리고 내리면서 삭제가 이루어진다면 Update Logic의 Lock
//Release Logic의 Lock 이 동시에 발생해 재귀적 Lock 문제가 발생할 수 있다. 하지만 해당 경우 항상 RecvProc에서 1을 문 상태로 로직이 돌기 때문에 문제가 되지 않는다고 판단하였다.
//Leave 호출 -> 이후 Release 호출 이므로 문제 X + 만약 중복해서 0이 된다 하더라도 Release Flag 때문에 접근이 불가능

void SoloInstance::RegisterRelease(NetSession* session)
{
	if (InterlockedCompareExchange(reinterpret_cast<long*>(session->GetIoCntPtr()), RELEASE_MASK, 0) != 0)
		return;

	//Release Overlapped를 IOCP Core로 PQCS를 쏜다. 
	_service->ReleasePost(session);
}

void SoloInstance::RegisterExit()
{
	_service->ExitLoopInstance();
}

void SoloInstance::RegisterAccept(uint64 session_id)
{
	NetSession* session = _service->GetSessionInLib(session_id);

	session->SetLogicInstance(this);
	OnEnter(session_id, session->GetSessionInstance());
}
