#pragma once
#include "ChatValues.h"
#include "SessionInstance.h"

struct Player : public SessionInstance
{
#pragma warning (disable : 26495)

	Player() : _sector_x(UINT16_MAX), _login_flag(false)
	{

	}

#pragma warning (default : 26495)

	__forceinline void InitPlayer(uint64 session_id, int64 account_num, WCHAR* id, WCHAR* nick_name)
	{
		_session_id = session_id;
		_account_num = account_num;
		wcscpy_s(_id, W_MAX_ID_LENGTH, id);
		wcscpy_s(_nick_name, W_MAX_NICKNAME_LENGTH, nick_name);
		_sector_x = UINT16_MAX;
		_login_flag = false;
		//_sector_y = UINT16_MAX;
	}

	int64 _account_num;
	WORD _sector_x;
	WORD _sector_y;
	WCHAR _id[20];
	WCHAR _nick_name[20];
	bool _login_flag;
};

