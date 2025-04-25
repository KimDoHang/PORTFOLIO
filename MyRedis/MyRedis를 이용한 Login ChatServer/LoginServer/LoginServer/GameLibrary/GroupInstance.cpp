#include "pch.h"
#include "GroupInstance.h"
#include "NetSession.h"
#include "SessionInstance.h"
#include "NetSerializeBuffer.h"
#include "NetService.h"
#include "LogicInstance.h"

GroupInstance::~GroupInstance()
{
}

void GroupInstance::Loop()
{
	bool ret = JobUpdate();

	if (ret == false)
		return;

	MsgUpdate();

	FrameUpdate();

	//Timer에 등록
	_service->RegisterLoop(static_cast<LoopInstance*>(this));
}

bool GroupInstance::JobUpdate()
{
	long size = 0;
	Job* job;

	while (true)
	{
		size = _job_queue.Size();

		if (size == 0)
			break;

		_job_queue.Dequeue(job);

		switch (job->type)
		{
		case JobType::AcceptJob:
		{
			//io_cnt를 물고 있는 상태이므로 삭제되지 않는다. 직접 접근 가능
			NetSession* session = _service->GetSessionInLib(job->session_id);

			//콘텐츠 등록
			_session_player_hash[job->session_id] = session;

			//Enter Job 호출 -> 상태 변경 후 이므로 정상적으로 WSARecv 가능
			OnEnter(job->session_id, session->GetSessionInstance());

			//멀티 -> 싱글 상황이 존재하기 때문에 좀 더 일찍 호출이 가능할듯.. 
			_service->ReturnSession(session);
			break;
		}
		case JobType::EnterJob:
		{
			//io_cnt를 물고 있는 상태이므로 삭제되지 않는다. 직접 접근 가능
			NetSession* session = _service->GetSessionInLib(job->session_id);

			//상태 선 변경
			session->SetLogicInstance(this);

			//콘텐츠 등록
			_session_player_hash[job->session_id] = session;

			//Enter Job 호출 -> 상태 변경 후 이므로 정상적으로 WSARecv 가능
			OnEnter(job->session_id, session->GetSessionInstance());
			
			//io_cnt 제거
			_service->ReturnSession(session);
			break;
		}
		case JobType::LeaveJob:
		{
			//RegisterLeave에서 1을 증가 시킨 상태로 처리하므로 절대 삭제 되기 전이다.
			NetSession* session = _service->GetSessionInLib(job->session_id);
			
			//OnLeave 및 컨텐츠 정리
			OnLeave(job->session_id, session->GetSessionInstance());
			_session_player_hash.erase(job->session_id);
			
			LogicInstance* logic_instance = _service->GetLogicInstance(job->next_dest);

			logic_instance->RegisterEnter(job->session_id, _logic_type);
			break;
		}
		case JobType::ReleaseJob:
		{
			NetSession* session = _service->GetSessionInLib(job->session_id);

			//Group 내부 정리
			_session_player_hash.erase(job->session_id);

			//Library session 정리
			_service->ReleaseProc(session);
			break;
		}
		case JobType::ExitJob:
		{
			JOB_FREE(job);
			_service->ExitLoopInstance();
			return false;
		}
		default:
		{
			WCHAR* lob_buff = g_logutils->GetLogBuff(L"JobType Not Defined");
			_service->OnError(dfLOG_LEVEL_SYSTEM, job->session_id, df_JOBTYPE_NOT_DEFINED_ERROR, lob_buff);
			break;
		}
		}
		JOB_FREE(job);
	}

	return true;
}

//전체 hash를 돌면서 처리한다. 
//Leave, Release 처리는 Job Queue에서 직렬적으로 처리가 이루어진다. -> 이때 hash에 대한 정보 또한 삭제
//만약 지금 hash를 순회하고 있다는 것은 결국 이러한 부분에서 삭제가 이루어지지 않았기 때문에 동일한 session에 대해 비록
//Release Job은 왔더라도 재활용은 되지 않은 것이다. 따라서 session에 대해 직접 접근하여 처리가 가능하다. (굳이 io_cnt를 올리지 않더라도)
//하지만 Release, Leave시에 바로 hash를 삭제하는 것이 불가능하므로 지연 삭제 혹은 Leave, Release Job을 던져준다.
void GroupInstance::MsgUpdate()
{
	NetSession* session;
	long size;

	auto begins = _session_player_hash.begin();
	auto ends = _session_player_hash.end();

	for (auto session_iter = begins; session_iter != ends; session_iter++)
	{
		session = session_iter->second;
		
		LockFreeQueueStatic<NetSerializeBuffer*>* msg_queue = session->GetMsgQueue();

		NetSerializeBuffer* msg;

		while (true)
		{
			size = msg_queue->Size();

			if (size == 0)
				break;

			msg_queue->Dequeue(msg);
			OnRecvMsg(session_iter->first, session->GetSessionInstance(), msg);

			FREE_NET_RECV_PACKET(msg);
		}

		if(session->SendPostFlag())
			_service->RegisterSessionSendPost(session_iter->first);
	}
}

void GroupInstance::FrameUpdate()
{
	OnFrame();
}

void GroupInstance::RecvMsg(NetSession* session, NetSerializeBuffer* msg)
{
	//Msg queue에 넣어준다. 
	msg->AddCnt();
	session->EnqueueMsg(msg);
}

void GroupInstance::RegisterAccept(uint64 session_id)
{
	NetSession* session = _service->GetSessionInLib(session_id);
	session->AddCntInLib();
	session->SetLogicInstance(this);

	Job* job = JOB_ALLOC();
	job->JobInit(JobType::AcceptJob, session_id);
	_job_queue.Enqueue(job);
}

//멀티에서는 이미 RecvProc에서 1을 물고 있으므로 올릴 필요가 없다.
//따라서 등록시에 올려준다. (멀티 -> 싱글, Accept -> 싱글)
//Accept 시에는 반드시 이전 상태를 멀티로 호출 + (Accept, Multi에서는 삭제 전이므로 그냥 io_cnt를 올려도된다. (이미 물고 있으므로 0이 될일이 없다)
void GroupInstance::RegisterEnter(uint64 session_id, LogicType prev_stream_type)
{
	if (prev_stream_type == LogicType::SoloType)
	{
		NetSession* session = _service->GetSessionInLib(session_id);
		session->AddCntInLib();
	}

	Job* job = JOB_ALLOC();
	job->JobInit(JobType::EnterJob,session_id);
	_job_queue.Enqueue(job);
}

void GroupInstance::RegisterLeave(uint64 session_id, uint16 logic_id)
{
	//싱글에서는 항상 1을 잡은 상태로 처리한다 실패시 반환 (이동 중 Release가 호출되는 것을 방지함으로써 Release 다음에 Leave가 오지 않도록 한다.)
	NetSession* session = _service->GetSession(session_id);
	if (session == nullptr)
		return;

	Job* job = JOB_ALLOC();
	job->JobInit(JobType::LeaveJob, session_id, logic_id);
	_job_queue.Enqueue(job);
}

//멀티와 다르게 싱글은 로직처리 중에 이것이 호출될 수 있으므로 반드시 job을 통해 넘겨주어야 한다.
void GroupInstance::RegisterRelease(NetSession* session)
{
	if (InterlockedCompareExchange(reinterpret_cast<long*>(session->GetIoCntPtr()), RELEASE_MASK, 0) != 0)
		return;

	Job* job = JOB_ALLOC();
	job->JobInit(JobType::ReleaseJob, session->GetSessionID());
	_job_queue.Enqueue(job);
}

void GroupInstance::RegisterExit()
{
	Job* job = JOB_ALLOC();
	job->type = JobType::ExitJob;
	_job_queue.Enqueue(job);
}



