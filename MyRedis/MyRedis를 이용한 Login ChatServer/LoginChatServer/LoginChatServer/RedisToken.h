#pragma once
#include "LockFreeObjectPool.h"

#define df_SESSION_KEY_BUFF_SIZE 30

struct TokenObject
{
	TokenObject()
	{

	}

	TokenObject(int64 token_key, uint64 session_id_val, char* session_key_val)
	{
		key = token_key;
		session_id = session_id_val;
		memcpy(session_key, session_key_val, 64);
	}

	int64 key;
	uint64 session_id;
	char session_key[64];
};

extern LockFreeObjectPool<TokenObject, true> _token_pool;
