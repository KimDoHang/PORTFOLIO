#pragma once

/*
	-- NetService
*/
//SendPost Arr Size

//LogUtils
const unsigned int max_log_size = 200;
const unsigned int max_log_num = 10000;
const int32 df_LOG_BUFF_SIZE = 200;


//NetAddress
const int32 IPBUF_SIZE = 20;

//Utils NAME_MAX_PATH
const int32 NAME_MAX_PATH = MAX_PATH / 2;

//LockFreeObjectPoolTLS ChunkNode Size
const int MAX_CHUNK_SIZE = 40;

//RecvPacket 배열 크기
const int32 eLAN_SERIALIZE_BUFFER_ARR_SIZE = 5;
const int32 eNET_SERIALIZE_BUFFER_ARR_SIZE = 200;

//RecvPacket의 직렬화 버퍼 크기
const int32 eLAN_RECV_BUFFER_DEFAULT = 500;
const int32 eNET_RECV_BUFFER_DEFAULT = 500;

//SendPacket의 직렬화 버퍼 크기
const int32 eLAN_SEND_BUFFER_DEFAULT = 500;
const int32 eNET_SEND_BUFFER_DEFAULT = 500;



////  MAX_QUEUE_SIZE
const int32 df_QUEUE_SIZE = 100000;


//Error 처리 관련 Value들
const int32 df_MAX_PACKET_LEN = UINT16_MAX;

//
// Session ARR
//
const int32 df_SEND_ARR_SIZE = 200;

const WCHAR* const LAN_LOG_DIR = L".\\LAN\\LOG\\";
const WCHAR* const LAN_PERFORMANCE_DIR = L".\\LAN\\PERFORMANCE\\";
const WCHAR* const LAN_PROFILE_DIR = L".\\LAN\\PROFILER\\";
const WCHAR* const LAN_FILE_NAME = L"CHAT";

const WCHAR* const NET_LOG_DIR = L".\\NET\\LOG\\";
const WCHAR* const NET_PERFORMANCE_DIR = L".\\NET\\PERFORMANCE\\";
const WCHAR* const NET_PROFILE_DIR = L".\\NET\\PROFILER\\";
const WCHAR* const NET_FILE_NAME = L"CHAT";


const uint32 RELEASE_MASK = (uint64)1 << 31;
