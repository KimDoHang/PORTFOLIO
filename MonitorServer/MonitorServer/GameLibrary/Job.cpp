#include "pch.h"
#include "Job.h"
#include "LogicInstance.h"
#include "GroupInstance.h"

LockFreeObjectPoolTLS<Job, false> g_job_pool;
