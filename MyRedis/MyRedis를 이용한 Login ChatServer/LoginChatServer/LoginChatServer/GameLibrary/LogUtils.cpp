#include "pch.h"
#include "LogUtils.h"
#include "Lock.h"

thread_local WCHAR LogUtils::_log_buff_tls[df_LOG_BUFF_SIZE];


LogUtils::LogUtils(const WCHAR* LAN_DIR_NAME, const WCHAR* NET_DIR_NAME)
{
	InitializeCriticalSection(&g_log_cs);

	g_iLogLevel = dfLOG_LEVEL_DEBUG;

	CreateLogDirect(LAN_DIR_NAME);
	CreateLogDirect(NET_DIR_NAME);
}

LogUtils::~LogUtils()
{
	DeleteCriticalSection(&g_log_cs);
}

void LogUtils::SetLogLevel(int32 log_level)
{
	g_iLogLevel = log_level;
}

int32 LogUtils::GetLogLevel()
{
	return g_iLogLevel;
}


void LogUtils::CreateLogDirect(const WCHAR* dir_name)
{
	//CSLock lock(&g_log_cs);
	//디렉토리 생성
	if (PathFileExistsW(dir_name) == false)
		if (CreateDirectory(dir_name, NULL) == 0)
			CRASH(L"CREATE DIR FAIL");
}


void LogUtils::Log(const WCHAR* param_dir_name, const WCHAR* param_file_name, const WCHAR* szString)
{

	time_t curTime = std::time(nullptr);
	tm curLocalTime;
	localtime_s(&curLocalTime, &curTime);


	WCHAR file_name[NAME_MAX_PATH];

	//자동으로 NULL문자를 추가해주며 버퍼가 부족한 경우 해당 크기만큼만을 저장한다.
	if (StringCchPrintfW(file_name, NAME_MAX_PATH, L"%sLOG_%s_%d_%02d_%02d.txt", param_dir_name, param_file_name, curLocalTime.tm_year + 1900, curLocalTime.tm_mon + 1, curLocalTime.tm_mday) != S_OK)
	{
		wcout << L"file_name buffer is small" << '\n';
	}

	WCHAR log_buf[LOG_MAX_PATH];

	if (StringCchPrintfW(log_buf, LOG_MAX_PATH, L"[%d][%d/%02d/%02d %02d:%02d:%02d] %s \n", AtomicIncrement32(&g_log_cnt), curLocalTime.tm_year + 1900, curLocalTime.tm_mon + 1, curLocalTime.tm_mday, curLocalTime.tm_hour, curLocalTime.tm_min, curLocalTime.tm_sec, szString) != S_OK)
	{
		wcout << L"log_buf buffer is small" << '\n';
	}


	{
		CSLock lock(&g_log_cs);

		if (PathFileExistsW(param_dir_name) == false)
			if (CreateDirectory(param_dir_name, NULL) == 0)
				CRASH(L"CREATE DIR FAIL");

		FILE* file;

		//a : 파일의 끝에서부터 기록이 이루어지며 없다면 파일을 생성한다.
		if (_wfopen_s(&file, file_name, L"a+") != 0)
		{
			wcout << L"File Open Fail Data -> " << log_buf << '\n';
			return;
		}

		if (file != nullptr)
		{
			fwprintf(file, log_buf);
			fclose(file);
			return;
		}

		wcout << "File PTR is null Data -> " << log_buf << '\n';
		fclose(file);
	}

	return;
}




