#pragma once

//Log Macro Level
#define dfLOG_LEVEL_DEBUG				0
#define dfLOG_LEVEL_ERROR				1
#define dfLOG_LEVEL_SYSTEM				2



//Error Code
#define df_THREAD_START					16001
#define df_THREAD_END					16002

#define df_SENDQ_FULL					16010
#define df_RECVQ_FULL				    16011
#define df_SESSION_FULL					16012
#define df_RECVQ_NOT_CLEAR				16013


//Packet Header Error
#define df_HEADER_CODE_ERROR			16020
#define df_HEADER_LEN_ERROR				16021
#define df_HEADER_CHECKSUM_ERROR		16022

#define df_OVERLAPPED_NOT_EQUAL			16030


#define df_JOBTYPE_NOT_DEFINED_ERROR	160


#define df_JOB_INSTANCETYPE_WRONG_ERROR	16051
#define df_JOB_SESSION_CNT_ERROR		16052
#define df_DEDINAME_ALREADY_DEFINED_ERROR    16053