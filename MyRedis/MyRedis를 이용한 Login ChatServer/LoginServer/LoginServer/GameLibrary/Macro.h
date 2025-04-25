#pragma once
#define CRASH(status) __debugbreak();


#define API_LOG(LogLevel, session_id, errCode, log_buff)					\
do																			\
{																			\
		errCode = WSAGetLastError();										\
		OnError(LogLevel, session_id, errCode, log_buff);					\
} while (0)

#define LIB_LOG(LogLevel, session_id, errCode, log_buff)					\
do																			\
{																			\
		OnError(LogLevel, session_id, errCode, log_buff);					\
} while (0)	

#define API_BREAKLOG(LogLevel, session_id, errCode, log_buff)				\
do																			\
{																			\
		errCode = WSAGetLastError();										\
		_thread_manager->SuspendThreadALL();								\
		OnError(LogLevel, session_id, errCode, log_buff);					\
		__debugbreak();														\
} while (0)	


#define LIB_BREAKLOG(LogLevel, session_id, errCode, log_buff)				\
do																			\
{																			\
		_thread_manager->SuspendThreadALL();								\
		OnError(LogLevel, session_id, errCode, log_buff);					\
		__debugbreak();														\
} while (0)	

#define API_SYSTEM_LOG(LogLevel, errCode, log_buff)							\
do																			\
{																			\
		errCode = WSAGetLastError();										\
		OnError(LogLevel, errCode, log_buff);								\
} while (0)

#define LIB_SYSTEM_LOG(LogLevel, errCode, log_buff)							\
do																			\
{																			\
		OnError(LogLevel, errCode, log_buff);								\
} while (0)	

#define API_SYSTEM_BREAKLOG(LogLevel, errCode, log_buff)					\
do																			\
{																			\
		errCode = WSAGetLastError();										\
		_thread_manager->SuspendThreadALL();								\
		OnError(LogLevel, errCode, log_buff);								\
		__debugbreak();														\
} while (0)	


#define LIB_SYSTEM_BREAKLOG(LogLevel, errCode, log_buff)					\
do																			\
{																			\
		_thread_manager->SuspendThreadALL();								\
		OnError(LogLevel, errCode, log_buff);								\
		__debugbreak();														\
} while (0)	




