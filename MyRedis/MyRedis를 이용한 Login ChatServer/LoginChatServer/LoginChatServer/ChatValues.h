#pragma once
#include "pch.h"

const int32 MAX_SECTOR_SIZE_X = 50;
const int32 MAX_SECTOR_SIZE_Y = 50;
const int32 MAX_TIME_OUT = 40000;

const int32 MAX_ID_LENGTH = 20 * sizeof(WCHAR);
const int32 MAX_NICKNAME_LENGTH = 20 * sizeof(WCHAR);
const int32 W_MAX_ID_LENGTH = 20;
const int32 W_MAX_NICKNAME_LENGTH = 20;
const int32 MAX_SESSIONKEY_LENGTH = 64;

const int dx[9] = { -1, -1, -1, 0,0,0,1,1,1 };
const int dy[9] = { -1,0,1,-1,0,1,-1,0,1 };

#define MAX_CNT_MSG 6000

#define df_CHAT_ACCOUNT_ERROR			17000
#define df_CHAT_SECTORIDX_ERROR			17001
#define df_CHAT_MAX_PLAYER_ERROR			17002
#define df_CHAT_NOTLOGIN_ERROR			17003
#define df_CHAT_MAX_PLAYER				17004

#define df_MONITOR_MAX_CONNECTION_ERROR 17010

enum ContentLogicID
{
	ChatGroupID = 0,
	MaxServerID,
};