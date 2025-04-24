#pragma once
#include <atomic>
#include <mutex>

using BYTE = unsigned char;
using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;

#define BUFSIZE 10000
#define WM_SOCKET (WM_USER + 1)

#define CRASH(status) __debugbreak();

#define dfLOG_LEVEL_DEBUG				0
#define dfLOG_LEVEL_ERROR				1
#define dfLOG_LEVEL_SYSTEM				2

#define HEART_BEAT_TIMER	10

#define df_MONITOR_FILE L"MonitorDistributionClient.cnf"
#define df_MONITOR_LOG_FILE	L"MonitorGUI"

const int block_size[4] = { 16,32,64,128 };


enum TileType
{
	NOTILE,
	START,
	DEST,
	BLOCK,
	VISITED,
	LOOK,
	MAXTILETYPE
};

enum Dir
{
	L,
	U,
	R,
	D,
	LU,
	RU,
	LD,
	RD,
	NoDIR,
};

enum BLOCK_SIZE
{
	BLOCK16,
	BLOCK32,
	BLOCK64,
	BLOCK128,
	MAX_BLCOK_SIZE,
};

struct Pos
{
	int x;
	int y;
	Pos(int X = -1, int Y = -1) : x(X), y(Y) {}

	bool operator== (const Pos& other)
	{
		return (x == other.x) && (y == other.y);
	}

	bool operator!= (const Pos& other)
	{
		return !(*this == other);
	}

	Pos& operator=(const Pos& other)
	{
		x = other.x;
		y = other.y;
		return *this;
	}

	Pos& operator+=(const Pos& other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}

	Pos& operator-=(const Pos& other)
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}

	Pos& operator*=(const int num)
	{
		x *= num;
		y *= num;
		return *this;
	}

	Pos operator+(const Pos& other)
	{
		Pos pos;
		pos.x = x + other.x;
		pos.y = y + other.y;
		return pos;
	}

	Pos operator-(const Pos& other)
	{
		Pos pos;
		pos.x = x - other.x;
		pos.y = y - other.y;
		return pos;
	}

	Pos operator*(const int num)
	{
		Pos pos;
		pos.x = x * num;
		pos.y = y * num;
		return pos;
	}
};


#define df_LOGIN_KEY_SIZE 32

enum ServerNo : uint16
{
	NoneServerNo = 0,
	LoginServerNo = 1,
	ChatServerNo = 2,
	GameServerNo = 3,
	HardwareServerNo = 4,
};