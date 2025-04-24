#pragma once
#include "pch.h"

enum PACKET_TYPE : uint16
{
	en_PACKET_CS_MONITOR_DISTRIBUTION = 26000,

	//------------------------------------------------------------
	// 모니터링 분포 클라이언트 로그인 요청
	//
	//	{
	//		WORD	Type		
	//		BYTE	ServerNo				// 서버 No
	//		char	LoginSessionKey[32]	
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_MONITOR_DISTRIBUTION_REQ_LOGIN,

	//------------------------------------------------------------
	//모니터링 서버 -> 모니터링 분포 클라이언트
	//
	//	{
	//		WORD	Type
	//		USHORT  XSize
	//		USHORT  YSize
	//		BYTE	Status				// 0:실패	1:성공
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_SC_MONITOR_DISTRIBUTION_RES_LOGIN,

	//------------------------------------------------------------
	// 모니터링 서버 -> 모니터링 분포 클라이언트
	//
	//	{
	//		WORD	Type
	//		int		TimeStamp
	//		USHORT  SectorInfo[Y][X]
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_SC_MONITOR_DISTRIBUTION_SECTOR_INFO,


	//------------------------------------------------------------
	// 하트비트
	//
	//	{
	//		WORD		Type
	//	}
	//
	//
	// 
	// 
	//------------------------------------------------------------	
	en_PACKET_CS_GUI_REQ_HEARTBEAT,


	en_PACKET_EXIT_SERVER,
};


