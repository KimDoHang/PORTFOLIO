#pragma once

#define df_LOGIN_KEY_SIZE 32
#define df_IP_BUFF_SIZE 20
#define df_QUERY_BUFF_SIZE 300
//LAN ErrorCode
#define df_LAN_PACKET_TYPE_ERROR		 20010
#define df_LAN_GET_SOCKET_ADDR_ERROR	 20011
#define df_LAN_SERVER_NO_ERROR			 20012
#define df_LAN_UPDATE_DATA_ERROR		 20013

//NET ErrCode
#define df_NET_PACKET_TYPE_ERROR		 25010
#define df_NET_GET_SOCKET_ADDR_ERROR	 25011
#define df_NET_SERVER_NO_ERROR			 25012


#define df_TABLE_NOT_EXIST_SQL_ERROR 1146


#define df_DB_TICK	600000
#define df_FRAME_TICK 1000

const char* ip_data = "127.0.0.1";


#define df_MONITORGUI_LOG_FILE_NAME	L"MonitorGUI_Server"
#define df_MONITORGUI_FILE_NAME L"MonitorGUI_Server.cnf"

const int32 MAX_SECTOR_SIZE_X = 50;
const int32 MAX_SECTOR_SIZE_Y = 50;



