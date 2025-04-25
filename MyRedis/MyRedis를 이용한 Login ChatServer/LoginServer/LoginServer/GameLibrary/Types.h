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

enum  ServiceType : uint8
{
	NoneInfoType = 0,
	LanServerType = 1,
	NetServerType = 2,
	LanClientType = 3,
	NetClientType = 4,
};

enum class OverlappedEventType : uint8
{
	ACCEPT_OVERLAPPED = 1,
	MAXACCEPT_OVERLAPPED = 2,
	RELEASE_OVERLAPPED = 3,
	SEND_OVERLAPPED = 4,
	RECV_OVERLAPPED = 5,
	SENDPOST_OVERLAPPED = 6,
	SESSION_POST_OVERLAPPED = 7,
	LOOP_OVERLAPPED = 8,
};

enum class JobType : uint16
{
	AcceptJob = 0,
	EnterJob = 1,
	LeaveJob = 2,
	ReleaseJob = 3,
	RegisterJob = 4,
	RemoveJob = 5,
	ExitJob = 6,
};

enum class LogicType : uint8
{
	SoloType = 1,
	GroupType = 2,
};


struct OverlappedEvent : public WSAOVERLAPPED
{
	OverlappedEventType type;
	void* instance;
};
