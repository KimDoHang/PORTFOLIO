#pragma once

enum PACKET_TYPE : uint16
{
	////////////////////////////////////////////////////////
	//
	//	Client & Server Protocol
	//
	////////////////////////////////////////////////////////

	//------------------------------------------------------
	// RedisServer
	//------------------------------------------------------
	en_PACKET_CS_REDIS_SERVER = 5000,


	//------------------------------------------------------------
	// 레디스서버 Set Job 보내기
	//
	//	{
	//		uint16	Type
	//
	//		uint16	ServerType
	//		int64	Key
	// 		uint16	ValLen
	//		char	Val[ValLen]		// null 미포함
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_REDIS_REQ_SET,

	//------------------------------------------------------------
	// 레디스서버 Set Job 응답
	//
	//	{
	//		uint16	Type
	//
	//		uint8	Status				// 0:실패	1:성공
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_REDIS_RES_SET,


	//------------------------------------------------------------
	// 레디스서버 Set expire Job 보내기
	//
	//	{
	//		uint16	Type
	//		
	//		uint16	ServerType
	//		int64	Key
	// 		int32   ExpireTime
	// 		uint16	ValLen
	//		char	Val[ValLen]		// null 미포함
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_REDIS_REQ_SET_EXPIRE,

	//------------------------------------------------------------
	// 레디스서버 Set expire Job 응답
	//
	//	{
	//		uint16	Type
	//	
	//		uint8	Status				// 0:실패	1:성공
	//	}
	//
	//------------------------------------------------------------
	en_PACKET_CS_REDIS_RES_SET_EXPIRE,

	//------------------------------------------------------------
	// 레디스서버 Get Job 보내기
	//
	//	{
	//		uint16	Type
	//
	//		uint16	ServerType
	//		int64	Key
	//	}
	//
	//------------------------------------------------------------

	en_PACKET_CS_REDIS_REQ_GET,

	//------------------------------------------------------------
	// 레디스서버 Get Job 응답
	//
	//	{
	//		uint16	Type
	//
	//		uint8	Status				// 0:실패	1:성공
	// 		uint16	ValLen
	//		char	Val[ValLen]		// null 미포함
	//	}
	//
	//------------------------------------------------------------

	en_PACKET_CS_REDIS_RES_GET,

	//------------------------------------------------------------
	// 레디스서버 Del Job 보내기
	//
	//	{
	//		uint16	Type
	//
	//		uint16	ServerType
	//		int64	Key
	//	}
	//
	//------------------------------------------------------------

	en_PACKET_CS_REDIS_REQ_DEL,

	//------------------------------------------------------------
	// 레디스서버 Del Job 보내기
	//
	//	{
	//		uint16	Type
	//
	//		uint8	Status				// 0:실패	1:성공
	//	}
	//
	//------------------------------------------------------------

	en_PACKET_CS_REDIS_RES_DEL,

};






