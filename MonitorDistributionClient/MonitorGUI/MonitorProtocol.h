#pragma once
#include "pch.h"

enum PACKET_TYPE : uint16
{
	en_PACKET_CS_MONITOR_DISTRIBUTION = 26000,

	//------------------------------------------------------------
	// ����͸� ���� Ŭ���̾�Ʈ �α��� ��û
	//
	//	{
	//		WORD	Type		
	//		BYTE	ServerNo				// ���� No
	//		char	LoginSessionKey[32]	
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_MONITOR_DISTRIBUTION_REQ_LOGIN,

	//------------------------------------------------------------
	//����͸� ���� -> ����͸� ���� Ŭ���̾�Ʈ
	//
	//	{
	//		WORD	Type
	//		USHORT  XSize
	//		USHORT  YSize
	//		BYTE	Status				// 0:����	1:����
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_SC_MONITOR_DISTRIBUTION_RES_LOGIN,

	//------------------------------------------------------------
	// ����͸� ���� -> ����͸� ���� Ŭ���̾�Ʈ
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
	// ��Ʈ��Ʈ
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


