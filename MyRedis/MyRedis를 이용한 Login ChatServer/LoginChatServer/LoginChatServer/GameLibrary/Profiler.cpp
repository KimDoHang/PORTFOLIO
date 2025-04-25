#include "pch.h"
#include "Profiler.h"

#pragma warning(disable:6387)

using namespace std;

thread_local Profiler::ProfilerTLS* Profiler::_tls_profiler = nullptr;
Profiler g_profiler;



void Profiler::InitProfiler()
{
	//clock 수 조절 및 freq 초기화
	timeBeginPeriod(1);
	::QueryPerformanceFrequency(&_freq);

	//전체 스레드 Prfile Table IDX
	_gb_table_cnt = 0;

	//Rest 기능을 위한 flag
	_reset_flag = 0;
	_profile_cnt = 0;

#ifdef PROFILE_AVERAGE
	InitializeCriticalSection(&_map_cs);
#endif
}

void Profiler::ProfileBegin(const WCHAR* szName)
{
	if (_tls_profiler == nullptr)
	{
		int32 table_idx = InterlockedIncrement(&_gb_table_cnt) - 1;
		_tls_profiler = new ProfilerTLS;
		_profiles_table[table_idx] = _tls_profiler;
	}

	if ((InterlockedIncrement(&_reset_flag) & df_PROFILE_RESET_ON_FLAG) != 0)
	{
		return;
	}

	//어차피 자신의 idx는 싱글로 동작하므로 딱히 interlock 처리를 하지 않았다.
	//테이블에 정보가 없는 경우 자신의 테이블 번호 부여 및 초기

	int idx = 0;
	ProfilerTLS* profiler_tls = _tls_profiler;
	//테이블에 ProfileSample 존재 여부 확인 -> 비교 줄이는 방법 고민해보기
	for (; idx < profiler_tls->_table_idx; idx++)
	{
		if (wcscmp(profiler_tls->_profile_sample_table[idx].tagName, szName) == 0)
		{
			break;
		}
	}

	//없는 경우 추가하는 코드
	if (idx == profiler_tls->_table_idx)
	{

		//최대 테이블 수가 고정이므로 이에 대한 초과시 에러 처리
		if (idx >= PROFILE_SAMPLE_TABLE_SIZE)
			__debugbreak();

		profiler_tls->_profile_sample_table[profiler_tls->_table_idx++].InitProfileSample(szName);
	}

	LARGE_INTEGER& startTime = profiler_tls->_profile_sample_table[idx].iStartTime;
	profiler_tls->_tls_profile_on = 1;

	//End 처리 이전에 Begin이 다시 호출되면 에러처리를 위한 코드
	if (startTime.QuadPart != 0)
	{
		__debugbreak();
	}

	//정확성을 위한 측정 시간을 최대한 늦추었다.
	QueryPerformanceCounter(&startTime);
}

void Profiler::ProfileEnd(const WCHAR* szName)
{
	LARGE_INTEGER endTime;
	QueryPerformanceCounter(&endTime);

	int idx = 0;
	ProfilerTLS* profiler_tls = _tls_profiler;

	if (profiler_tls->_tls_profile_on == 0)
		return;

	for (; idx < profiler_tls->_table_idx; idx++)
	{
		if (wcscmp(profiler_tls->_profile_sample_table[idx].tagName, szName) == 0)
		{
			break;
		}
	}

	//테이블에 해당 Name이 존재하지 않는 경우 에러처리
	if (idx == profiler_tls->_table_idx)
		__debugbreak();

	ProfileSample& ps = profiler_tls->_profile_sample_table[idx];

	//End가 연속으로 호출되는 것에 대한 에러 처리
	if (ps.iStartTime.QuadPart == 0)
		__debugbreak();


	endTime.QuadPart -= ps.iStartTime.QuadPart;

	//최대 및 최소 값에 대한 예외 처리이다.
	if (ps.iMax[0] < endTime.QuadPart)
	{
		ps.iMax[1] = ps.iMax[0];
		ps.iMax[0] = endTime.QuadPart;
	}
	else if (ps.iMax[1] < endTime.QuadPart)
		ps.iMax[1] = endTime.QuadPart;

	if (ps.iMin[0] > endTime.QuadPart)
	{
		ps.iMin[1] = ps.iMin[0];
		ps.iMin[0] = endTime.QuadPart;
	}
	else if (ps.iMin[1] > endTime.QuadPart)
		ps.iMin[1] = endTime.QuadPart;

	ps.iStartTime.QuadPart = 0;
	ps.iTotalTime += endTime.QuadPart;
	ps.iCall++;

	//되돌리기 - Reset 접근 가능하도록 만들어준다.
	profiler_tls->_tls_profile_on = 0;
	InterlockedDecrement(&_reset_flag);
}

void Profiler::ProfileDataOutText(const WCHAR* dirName, const WCHAR* szFileName)
{
	if ((InterlockedIncrement(&_reset_flag) & df_PROFILE_RESET_ON_FLAG) != 0)
	{
		return;
	}

	if (PathFileExistsW(dirName) == false)
		if (CreateDirectory(dirName, NULL) == 0)
			CRASH(L"CREATE DIR FAIL");


	time_t curTime = std::time(nullptr);
	tm curLocalTime;
	localtime_s(&curLocalTime, &curTime);

	WCHAR file_name[NAME_MAX_PATH];

	//자동으로 NULL문자를 추가해주며 버퍼가 부족한 경우 해당 크기만큼만을 저장한다.
	if (StringCchPrintfW(file_name, NAME_MAX_PATH, L"%s\\%s.txt", dirName, szFileName) != S_OK)
	{
		wcout << L"file_name buffer is small" << '\n';
	}

	FILE* file;

	if (_wfopen_s(&file, file_name, L"a") != 0)
	{
		__debugbreak();
	}

	int gb_table_cnt = _gb_table_cnt;

	fwprintf(file, L"###############################################################################################################################################################################################################\n");
	fwprintf(file, L"%4d/%02d/%02d_%02d:%02d:%02d\n", curLocalTime.tm_year + 1900, curLocalTime.tm_mon + 1, curLocalTime.tm_mday, curLocalTime.tm_hour, curLocalTime.tm_min, curLocalTime.tm_sec);
	fwprintf(file, L"---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
	fwprintf(file, L"| %-100s | %-15s | %-15s | %-15s | %-10s |\n", L"Name", L"Average", L"Min", L"Max", L"Call");

	for (int tables_idx = 0; tables_idx < gb_table_cnt; tables_idx++)
	{

		fwprintf(file, L"---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
		fwprintf_s(file, L"			- [%d] ThreadID : %d - 							 \n", tables_idx, _profiles_table[tables_idx]->_thread_id);
		fwprintf(file, L"---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
		int sample_table_idx = _profiles_table[tables_idx]->_table_idx;
		ProfileSample* ps_table = _profiles_table[tables_idx]->_profile_sample_table;

		for (int idx = 0; idx < sample_table_idx; idx++)
		{
			ProfileSample* ps = &ps_table[idx];

#if PROFILE_UNIT == USE_NANO
			if (ps->iCall == 0)
			{
				continue;
			}
			else if (ps->iCall <= 4)
			{

				fwprintf_s(file, L"%-100s | %-15llu(%cs) | %-15llu(%cs) | %-15llu(%cs) | %-10s(%lld)\n", ps->tagName, (ps->iTotalTime) * (TIME_UNIT / _freq.QuadPart) / ps->iCall, c_unit,
					(unsigned long long)ps->iMin[0] * (TIME_UNIT / _freq.QuadPart), c_unit, ps->iMax[0] * (TIME_UNIT / _freq.QuadPart), c_unit, SMALL_CALL, ps->iCall);
			}
			else
			{
				int64 totalTimeExceptAbnormal = ps->iTotalTime - ps->iMax[0] - ps->iMax[1] - ps->iMin[0] - ps->iMin[1];
				fwprintf_s(file, L"%-100s | %-15llu(%cs) | %-15llu(%cs) | %-15llu(%cs) | %-10lld\n", ps->tagName, totalTimeExceptAbnormal * (TIME_UNIT / _freq.QuadPart) / (ps->iCall - 4), c_unit,
					(unsigned long long)ps->iMin[0] * (TIME_UNIT / _freq.QuadPart), c_unit, ps->iMax[0] * (TIME_UNIT / _freq.QuadPart), c_unit, ps->iCall);
			}

#else
			if (ps->iCall == 0)
			{
				continue;
			}
			else if (ps->iCall <= 4)
			{

				fwprintf_s(file, L"%-100s | %-15.5lf(%cs) | %-15.5lf(%cs) | %-15.5lf(%cs) | %-10s(%lld)\n", ps->tagName, (ps->iTotalTime) / (double)(_freq.QuadPart / TIME_UNIT) / ps->iCall, c_unit,
					ps->iMin[0] / (double)(_freq.QuadPart / TIME_UNIT), c_unit, ps->iMax[0] / (double)(_freq.QuadPart / TIME_UNIT), c_unit, SMALL_CALL, ps->iCall);
			}
			else
			{
				int64 totalTimeExceptAbnormal = ps->iTotalTime - ps->iMax[0] - ps->iMax[1] - ps->iMin[0] - ps->iMin[1];
				fwprintf_s(file, L"%-100s | %-15.5lf(%cs) |%-15.5lf(%cs) |%-15.5lf(%cs) |%-10lld\n", ps->tagName, totalTimeExceptAbnormal / (double)(_freq.QuadPart / TIME_UNIT) / (ps->iCall - 4), c_unit,
					ps->iMin[0] / (double)(_freq.QuadPart / TIME_UNIT), c_unit, ps->iMax[0] / (double)(_freq.QuadPart / TIME_UNIT), c_unit, ps->iCall);
			}

#endif
		}
		fwprintf(file, L"---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
	}
	fwprintf(file, L"\n\n");

	fflush(file);
	fclose(file);

	InterlockedDecrement(&_reset_flag);
}

void Profiler::ProfileReset(void)
{
	while (InterlockedCompareExchange(&_reset_flag, df_PROFILE_RESET_ON_FLAG, 0))
	{

	}

	for (int i = 0; i < _gb_table_cnt; i++)
	{

		for (int idx = 0; idx < _profiles_table[i]->_table_idx; idx++)
		{
			ProfileSample* ps = &_profiles_table[i]->_profile_sample_table[idx];

			ps->iTotalTime = 0;
			ps->iCall = 0;
			ps->iMax[0] = -1;
			ps->iMax[1] = -1;
			ps->iMin[0] = INT64_MAX;
			ps->iMin[1] = INT64_MAX;
			ps->iStartTime.QuadPart = 0;
		}
	}

	InterlockedAnd(reinterpret_cast<long*>(&_reset_flag), df_PROFILE_RESET_OFF_FLAG);
}
