#pragma once

//사용시 반드시 객체를 선언해야만 한다. 만약 객체명을 두지 않는다면 임시 객체로 생성되어
//해당 라인에서 곧바로 소멸자가 호출된다.
#define PROFILE
//#define PROFILE_AVERAGE

enum PROFILE_UNIT_ENUM
{
	USE_NANO = 0,
	USE_MICRO = 1,
	USE_MS = 2,
};

const int32 PROFILE_UNIT = USE_MICRO;

#if PROFILE_UNIT == USE_NANO
const unsigned long long TIME_UNIT = 1000'000'000;
const char c_unit = 'n';
#elif PROFILE_UNIT == USE_MICRO
const unsigned long long TIME_UNIT = 1000000;
const char c_unit = 'u';
#else
const unsigned long long TIME_UNIT = 1000;
const char c_unit = 'm';
#endif


class Profiler
{
#pragma warning(disable:26495)


	//순서대로 단위, Profile 항목 이름 길이, 파일 이름 길이, 각 스레드 Profile 항목 수, 최대 스레드 수를 설정하는 크기이다.
	static const int PROFILE_NAME_SIZE = 64;
	static const int PROFILE_FILE_NAME_SIZE = 64;
	static const int PROFILE_SAMPLE_TABLE_SIZE = 64;
	static const int PROFILE_THREAD_TABLE_SIZE = 100;

	//SMALL_CALL 4번 이하의 동일 이름에 대한 호출시 기록
	const WCHAR* SMALL_CALL = L"Small Call";
	static const uint32 df_PROFILE_RESET_ON_FLAG = ((uint32)1 << 31);
	static const uint32 df_PROFILE_RESET_OFF_FLAG = ((uint32)1 << 31) - 1;
	struct ProfileSample
	{
		ProfileSample()
		{

		}
		ProfileSample(const WCHAR* funcTag)
		{
			wcscpy_s(tagName, PROFILE_NAME_SIZE, funcTag);
		}

		__forceinline void InitProfileSample(const WCHAR* funcTag)
		{
			wcscpy_s(tagName, PROFILE_NAME_SIZE, funcTag);
			iTotalTime = 0;
			iStartTime.QuadPart = 0;
			iMin[0] = MAXINT64;
			iMin[1] = MAXINT64;
			iMax[0] = -1;
			iMax[1] = -1;
			iCall = 0;
		}

		WCHAR  tagName[PROFILE_NAME_SIZE];
		LARGE_INTEGER iStartTime;
		int64 iTotalTime;
		int64 iMin[2];
		int64 iMax[2];
		int64 iCall;

		__forceinline bool operator<(const ProfileSample& other)
		{
			if (iCall == other.iCall)
				return iTotalTime > other.iTotalTime;

			return iCall > iCall;
		}

		__forceinline bool operator>(const ProfileSample& other)
		{
			if (iCall == other.iCall)
				return iTotalTime < other.iTotalTime;

			return iCall < iCall;
		}

	};

	struct ProfilerTLS
	{
		ProfilerTLS() : _table_idx(0), _thread_id(GetCurrentThreadId()), _tls_profile_on(0)
		{

		}

		~ProfilerTLS() = default;

		int32 _table_idx;
		int32 _thread_id;
		ProfileSample _profile_sample_table[PROFILE_SAMPLE_TABLE_SIZE];
		char _tls_profile_on;
	};
public:
	Profiler()
	{
		InitProfiler();
	}


	static bool FuncCompare(ProfileSample* other1, ProfileSample* other2)
	{
		if (other1->iCall == other2->iCall)
		{
			if (other1->iCall <= 4)
				return false;
			int64 totalTimeExceptAbnormal1 = other1->iTotalTime - other1->iMax[0] - other1->iMax[1] - other1->iMin[0] - other1->iMin[1];
			int64 totalTimeExceptAbnormal2 = other2->iTotalTime - other2->iMax[0] - other2->iMax[1] - other2->iMin[0] - other2->iMin[1];

			return totalTimeExceptAbnormal1 > totalTimeExceptAbnormal2;
		}

		else
		{
			return other1->iCall > other2->iCall;
		}
	}

	void InitProfiler();
	void ProfileBegin(const WCHAR* szName);
	void ProfileEnd(const WCHAR* szName);
	void ProfileDataOutText(const WCHAR* dirName, const WCHAR* szFileName);
	void ProfileReset(void);

private:
	LARGE_INTEGER _freq;
	WCHAR _profile_dir_name[NAME_MAX_PATH];
	ProfilerTLS* _profiles_table[PROFILE_THREAD_TABLE_SIZE];
	long _gb_table_cnt;
	long _profile_cnt;
	alignas(64) uint32 _reset_flag;

	static thread_local ProfilerTLS* _tls_profiler;
#ifdef PROFILE_AVERAGE
	std::map<const WCHAR*, ProfileSample*> _profiles_map;
	CRITICAL_SECTION _map_cs;
#endif
};

extern Profiler g_profiler;

#ifdef PROFILE
#define PRO_BEGIN(TagName)		g_profiler.ProfileBegin(TagName)
#define PRO_END(TagName)		g_profiler.ProfileEnd(TagName)
#define PRO_OUT(DirName, FileName)		g_profiler.ProfileDataOutText(DirName, FileName)
#define PRO_RESET()				g_profiler.ProfileReset()
#else	
#define PRO_BEGIN(TagName)
#define PRO_END(TagName)
#define PRO_OUT(FileName)		
#define PRO_RESET()				
#endif // PROFILE

class Profile
{
public:
	Profile(const WCHAR* funcName) : _funcName(funcName)
	{
		PRO_BEGIN(_funcName);
	}
	~Profile()
	{
		PRO_END(_funcName);
	}
private:
	const WCHAR* _funcName;
};

