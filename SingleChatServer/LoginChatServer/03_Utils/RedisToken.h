#pragma once
#include "LockFreeObjectPool.h"

#define df_SESSION_KEY_BUFF_SIZE 30

struct TokenObject
{
#pragma warning (disable : 26495)

	TokenObject()
	{

	}

#pragma warning (default : 26495)


	TokenObject(uint64 session_id_val, char* session_key_val)
	{
		session_id = session_id_val;
		memcpy(session_key, session_key_val, 64);
	}

	uint64 session_id;
	char session_key[64];
};

extern LockFreeObjectPool<TokenObject, true> _token_pool;
