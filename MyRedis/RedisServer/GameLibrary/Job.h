#pragma once
#include "LockFreeObjectPoolTLS.h"
#include "LoopInstance.h"

struct Job
{
	JobType type;
	LoopInstance* instance;
	uint64 session_id;
	uint16 next_dest;

	void JobInit(JobType param_type, LoopInstance* param_instance)
	{
		type = param_type;
		instance = param_instance;
	}

	void JobInit(JobType param_type, uint64 param_session_id)
	{
		type = param_type;
		session_id = param_session_id;
	}

	void JobInit(JobType param_type, uint64 param_session_id, uint16 param_next_dest)
	{
		type = param_type;
		session_id = param_session_id;
		next_dest = param_next_dest;
	}

};

extern LockFreeObjectPoolTLS<Job, false> g_job_pool;

#define JOB_ALLOC() g_job_pool.Alloc();
#define JOB_FREE(job) g_job_pool.Free(job);

