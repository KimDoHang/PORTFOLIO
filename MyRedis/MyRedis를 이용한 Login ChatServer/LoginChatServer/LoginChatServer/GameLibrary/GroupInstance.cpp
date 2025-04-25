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

	//Timer�� ���
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
			//io_cnt�� ���� �ִ� �����̹Ƿ� �������� �ʴ´�. ���� ���� ����
			NetSession* session = _service->GetSessionInLib(job->session_id);

			//������ ���
			_session_player_hash[job->session_id] = session;

			//Enter Job ȣ�� -> ���� ���� �� �̹Ƿ� ���������� WSARecv ����
			OnEnter(job->session_id, session->GetSessionInstance());

			//��Ƽ -> �̱� ��Ȳ�� �����ϱ� ������ �� �� ���� ȣ���� �����ҵ�.. 
			_service->ReturnSession(session);
			break;
		}
		case JobType::EnterJob:
		{
			//io_cnt�� ���� �ִ� �����̹Ƿ� �������� �ʴ´�. ���� ���� ����
			NetSession* session = _service->GetSessionInLib(job->session_id);

			//���� �� ����
			session->SetLogicInstance(this);

			//������ ���
			_session_player_hash[job->session_id] = session;

			//Enter Job ȣ�� -> ���� ���� �� �̹Ƿ� ���������� WSARecv ����
			OnEnter(job->session_id, session->GetSessionInstance());
			
			//io_cnt ����
			_service->ReturnSession(session);
			break;
		}
		case JobType::LeaveJob:
		{
			//RegisterLeave���� 1�� ���� ��Ų ���·� ó���ϹǷ� ���� ���� �Ǳ� ���̴�.
			NetSession* session = _service->GetSessionInLib(job->session_id);
			
			//OnLeave �� ������ ����
			OnLeave(job->session_id, session->GetSessionInstance());
			_session_player_hash.erase(job->session_id);
			
			LogicInstance* logic_instance = _service->GetLogicInstance(job->next_dest);

			logic_instance->RegisterEnter(job->session_id, _logic_type);
			break;
		}
		case JobType::ReleaseJob:
		{
			NetSession* session = _service->GetSessionInLib(job->session_id);

			//Group ���� ����
			_session_player_hash.erase(job->session_id);

			//Library session ����
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

//��ü hash�� ���鼭 ó���Ѵ�. 
//Leave, Release ó���� Job Queue���� ���������� ó���� �̷������. -> �̶� hash�� ���� ���� ���� ����
//���� ���� hash�� ��ȸ�ϰ� �ִٴ� ���� �ᱹ �̷��� �κп��� ������ �̷������ �ʾұ� ������ ������ session�� ���� ���
//Release Job�� �Դ��� ��Ȱ���� ���� ���� ���̴�. ���� session�� ���� ���� �����Ͽ� ó���� �����ϴ�. (���� io_cnt�� �ø��� �ʴ���)
//������ Release, Leave�ÿ� �ٷ� hash�� �����ϴ� ���� �Ұ����ϹǷ� ���� ���� Ȥ�� Leave, Release Job�� �����ش�.
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
	//Msg queue�� �־��ش�. 
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

//��Ƽ������ �̹� RecvProc���� 1�� ���� �����Ƿ� �ø� �ʿ䰡 ����.
//���� ��Ͻÿ� �÷��ش�. (��Ƽ -> �̱�, Accept -> �̱�)
//Accept �ÿ��� �ݵ�� ���� ���¸� ��Ƽ�� ȣ�� + (Accept, Multi������ ���� ���̹Ƿ� �׳� io_cnt�� �÷����ȴ�. (�̹� ���� �����Ƿ� 0�� ������ ����)
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
	//�̱ۿ����� �׻� 1�� ���� ���·� ó���Ѵ� ���н� ��ȯ (�̵� �� Release�� ȣ��Ǵ� ���� ���������ν� Release ������ Leave�� ���� �ʵ��� �Ѵ�.)
	NetSession* session = _service->GetSession(session_id);
	if (session == nullptr)
		return;

	Job* job = JOB_ALLOC();
	job->JobInit(JobType::LeaveJob, session_id, logic_id);
	_job_queue.Enqueue(job);
}

//��Ƽ�� �ٸ��� �̱��� ����ó�� �߿� �̰��� ȣ��� �� �����Ƿ� �ݵ�� job�� ���� �Ѱ��־�� �Ѵ�.
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



