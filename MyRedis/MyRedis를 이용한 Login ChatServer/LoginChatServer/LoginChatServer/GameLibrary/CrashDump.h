#pragma once
#include <iostream>
#include <crtdbg.h>
#pragma comment(lib, "DbgHelp.Lib")
#include <dbghelp.h>
#include <stdio.h>
#include <Psapi.h>


#ifndef _CRASH_DUMP_
#define _CRASH_DUMP_

class CCrashDump
{
public:

	CCrashDump()
	{

		_invalid_parameter_handler old_handler, new_handler;
		new_handler = myInvalidParameterHandler;

		old_handler = _set_invalid_parameter_handler(new_handler);
		_CrtSetReportMode(_CRT_WARN, 0);
		_CrtSetReportMode(_CRT_ASSERT, 0);
		_CrtSetReportMode(_CRT_ERROR, 0);

		_CrtSetReportHook(_custom_Report_hook);

		_set_purecall_handler(myPurecallHandler);

		SetHandlerDump();

	}

	static void Crash(void)
	{
		__debugbreak();
	}

	static LONG WINAPI MyExceptionFilter(__in PEXCEPTION_POINTERS p_exception_pointer)
	{
		SYSTEMTIME st_cur_time;

		long dump_cnt = InterlockedIncrement(&_dump_cnt);

		//PROCESS_MEMORY_COUNTERS pmc;


		WCHAR filename[MAX_PATH];

		GetLocalTime(&st_cur_time);

		wsprintf(filename, L"Dum_%d%02d%02d_%02d.%02d.%02d_%d.dmp", st_cur_time.wYear, st_cur_time.wMonth, st_cur_time.wDay, st_cur_time.wHour, st_cur_time.wMinute, st_cur_time.wSecond, dump_cnt);


		WCHAR log_file_name[MAX_PATH];
		HANDLE f_handle;

		wsprintf(log_file_name, L"Dum_%d%02d%02d_%02d.%02d.%02d_%d_ThreadLog.txt", st_cur_time.wYear, st_cur_time.wMonth, st_cur_time.wDay, st_cur_time.wHour, st_cur_time.wMinute, st_cur_time.wSecond, dump_cnt);
		f_handle = ::CreateFile(log_file_name, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (f_handle == INVALID_HANDLE_VALUE)
			cout << "Fail" << '\n';

		cout << "Call ThreadID" << GetCurrentThreadId() << '\n';

		wprintf(L"\n\n\n!!! Crash Error!!! %d.%d.%d / %d:%d:%d\n", st_cur_time.wYear, st_cur_time.wMonth, st_cur_time.wDay, st_cur_time.wHour, st_cur_time.wMinute, st_cur_time.wSecond);
		wprintf(L"Saving Dump File...\n");

		HANDLE h_dump_file = ::CreateFile(filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (h_dump_file != INVALID_HANDLE_VALUE)
		{
			_MINIDUMP_EXCEPTION_INFORMATION minidump_exception_information;
			minidump_exception_information.ThreadId = ::GetCurrentThreadId();
			minidump_exception_information.ExceptionPointers = p_exception_pointer;
			minidump_exception_information.ClientPointers = TRUE;

			MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), h_dump_file, MiniDumpWithFullMemory, &minidump_exception_information, NULL, NULL);

			CloseHandle(h_dump_file);

			wprintf(L"CrashDump Save Fin");
		}

		return EXCEPTION_EXECUTE_HANDLER;
	}

	static void SetHandlerDump()
	{
		SetUnhandledExceptionFilter(MyExceptionFilter);
	}

	static void myInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t p_reserved)
	{
		Crash();
	}

	static int _custom_Report_hook(int ireposttype, char* message, int* returnvalue)
	{
		Crash();
		return true;
	}

	static void myPurecallHandler(void)
	{
		Crash();
	}


private:

	static long _dump_cnt;

};


#endif // !_CRASH_DUMP_


//Dump Log를 어떻게 처리할 것인가???