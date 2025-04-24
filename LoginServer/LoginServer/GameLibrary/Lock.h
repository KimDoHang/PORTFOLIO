#pragma once
class SrwLockExclusiveGuard
{
public:
	SrwLockExclusiveGuard(SRWLOCK* lock) : _lock(lock)
	{
		AcquireSRWLockExclusive(_lock);
	}

	~SrwLockExclusiveGuard()
	{
		ReleaseSRWLockExclusive(_lock);
	}

private:
	SRWLOCK* _lock;
};

class SrwLockSharedGuard
{
public:
	SrwLockSharedGuard(SRWLOCK* lock) : _lock(lock)
	{
		AcquireSRWLockShared(_lock);
	}

	~SrwLockSharedGuard()
	{
		ReleaseSRWLockShared(_lock);
	}

private:
	SRWLOCK* _lock;
};

class CSLock
{
public:
	CSLock(CRITICAL_SECTION* lock) : _lock(lock)
	{
		EnterCriticalSection(_lock);
	}

	~CSLock()
	{
		LeaveCriticalSection(_lock);
	}

private:
	CRITICAL_SECTION* _lock;
};