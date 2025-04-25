#pragma once
#include "NetSerializeBuffer.h"

enum CHAT_GROUP_JOB_TYPE
{
	en_JOB_CS_CHAT_SERVER = 0,

	en_JOB_CREATE_SESSION,

	en_JOB_MONITOR,

};

struct ChatGroupJob
{
	static const WORD CHAT_GROUP_JOB_TYPE_SIZE = sizeof(WORD);

	__forceinline void JobInit(WORD type, uint64 session_id)
	{
		_session_id = session_id;
		_type = type;
	}

	__forceinline void JobInit(WORD type, uint64 session_id, bool login_success)
	{
		_session_id = session_id;
		_type = type;
		_login_success = login_success;
	}

	WORD _type;
	uint64 _session_id;
	bool _login_success;
};

