#include "pch.h"
#include "SoloInstance.h"
#include "NetSession.h"
#include "NetService.h"
#include "SessionInstance.h"

SoloInstance::~SoloInstance()
{
}


//��Ƽ�̹Ƿ� �׻� io_cnt�� 1�� ���� �����Ƿ� session_id ���� ���� ����

void SoloInstance::RecvMsg(NetSession* session, NetSerializeBuffer* msg)
{
	OnRecvMsg(session->GetSessionID(), session->GetSessionInstance(), msg);
}

//�̱� -> ��Ƽ (�������� 1�� �ø� �� ����), ��Ƽ -> ��Ƽ (������ RecvProc�̹Ƿ� ���� �ø��� �ʴ��� session ���Ű� �̷������ �ʴ´�.
//���� ������ ���°� �̱��̶�� cnt�� �����ִ� �۾����� �����Ѵ�. �̋� �ش� �۾��� Single Dedi SessionInstance �������� �Ͼ��.
//���� OnEnter ȣ��� RecvMsg ó���� ���ÿ� �̷���� �� �մµ� �̴� �ݵ�� OnEnter���� Enter Msg�� �������� ���������� �� �ذ��� �����ϴ�.
//���� OnEnter���� cnt�� ��� �ִµ� �̴� ������ Session�� ��ȯ�� ��� ���������� Single������ session_cnt�� ���� �ʱ� ������
//OnEnter�� OnRelease�� ���ÿ� ȣ��Ǵ� ���� �����ϴ�. -> ��Ƽ�� ���°� ���� �����Ƿ� IOCP Worker���� ��ٷ� OnRelease ȣ��, OnEnter������ ������ session_id�� ���� ó��
//�̷������. ���� �̸� ���� ���ؼ� OnEnter ���� session_cnt�� ���� ���״�.
void SoloInstance::RegisterEnter(uint64 session_id, LogicType prev_stream_type)
{
	NetSession* session = _service->GetSessionInLib(session_id);

	session->SetLogicInstance(this);
	OnEnter(session_id, session->GetSessionInstance());

	if (prev_stream_type == LogicType::GroupType)
		_service->ReturnSession(session);
}

//��Ƽ������ �׻� session_cnt�� ���� �����Ƿ� ���� session�� ������ �����ϴ�.
void SoloInstance::RegisterLeave(uint64 session_id, uint16 logic_id)
{
	NetSession* session = _service->GetSessionInLib(session_id);

	//OnLeave �� ������ ����
	OnLeave(session_id, session->GetSessionInstance());

	//LogicArr�κ��� ���� Content Class�� ã�� �� �ش� Ŭ������ Enter �Լ� ȣ��
	_service->GetLogicInstance(logic_id)->RegisterEnter(session_id, _logic_type);
}

//������ ResetPost, ReleasePost ��� �� �������� �´� ReleasePost�� ȣ��Ǿ�� �Ѵ�.
//���� io_cnt�� 0�̵Ǿ��� �� �� �������� Release�� ȣ��ȴ�.
//�̶� Release�� ȣ��Ǵ� ���� �ݵ�� Ư�� ������ ������ ���Ե� �����̰� ��Ȱ���� �ȵǹǷ� (id �L�����̹Ƿ�)
//session�� ���� ���� ������ �����ϴ�.
//��Ƽ�� ��� Register���� �ٷ� ReleaseProc�� ȣ�� ������ ���� RecvProc�� ���� ������ ó���ϰ� �ִ� �߿��� ������ �Ұ����ϴ�.
//���� ȣ��Ǵ� ���� �ش� �Լ����� ���� ȣ���ϴ� �� �ۿ� �������� �ʴ´�. ���� �ٷ� ó���� �����ϴ�.
//��� Lock�� �߻��ϴ� ���� -> Update���� ó���� io_cnt�� ���� �ʰ� ó���� �̷�����ٰ� SendPacket������ io_cnt�� �ø��� �����鼭 ������ �̷�����ٸ� Update Logic�� Lock
//Release Logic�� Lock �� ���ÿ� �߻��� ����� Lock ������ �߻��� �� �ִ�. ������ �ش� ��� �׻� RecvProc���� 1�� �� ���·� ������ ���� ������ ������ ���� �ʴ´ٰ� �Ǵ��Ͽ���.
//Leave ȣ�� -> ���� Release ȣ�� �̹Ƿ� ���� X + ���� �ߺ��ؼ� 0�� �ȴ� �ϴ��� Release Flag ������ ������ �Ұ���

void SoloInstance::RegisterRelease(NetSession* session)
{
	if (InterlockedCompareExchange(reinterpret_cast<long*>(session->GetIoCntPtr()), RELEASE_MASK, 0) != 0)
		return;

	//Release Overlapped�� IOCP Core�� PQCS�� ���. 
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
