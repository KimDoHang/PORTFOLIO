#pragma once


//�⺻ Dir ��ġ�� ���� ���������� LOG Dir�� �����ȴ�.
//������ ������ ServerName, IDX, CriticalSection�� �����ؾ� �Ѵ�./
//Dir�� ��� ���� LOCK�� ���� �����Ƿ� Server ó���� �ѹ��� Dir ���� �����ϰ� �ʹٸ� ������ �̷������ �Ѵ�.

class LogUtils
{
	//static const int32 NAME_MAX_PATH = MAX_PATH / 2;
	static const int32 LOG_MAX_PATH = 200;
public:
	LogUtils(const WCHAR* LAN_DIR_NAME = L".\\LAN\\", const WCHAR* NET_DIR_NAME = L".\\NET\\");
	~LogUtils();

	void CreateLogDirect(const WCHAR* dir_name);
	void SetLogLevel(int32 log_level);
	int32 GetLogLevel();

	void Log(const WCHAR* param_dir_name, const WCHAR* param_file_name, const WCHAR* szString);

	WCHAR* GetLogBuffPtr() { return _log_buff_tls; }

	template<typename... Args>
	WCHAR* GetLogBuff(const WCHAR* format, Args... args)
	{
		wsprintf(_log_buff_tls, format, args...);
		return _log_buff_tls;
	}

	template<typename... Args>
	void PrintLibraryLog(const WCHAR* param_dir_name, const WCHAR* param_file_name, const WCHAR* format, Args... args)
	{
		WCHAR log_buf[df_LOG_BUFF_SIZE];

		if (StringCchPrintfW(log_buf, df_LOG_BUFF_SIZE, format, args...) != S_OK)
		{
			//wcout << L"log_buf buffer is small" << '\n';
		}

		Log(param_dir_name, param_file_name, log_buf);
	}

private:
	int32 g_iLogLevel;
	int32 g_log_cnt;
	CRITICAL_SECTION g_log_cs;
	static thread_local WCHAR _log_buff_tls[df_LOG_BUFF_SIZE];
};


