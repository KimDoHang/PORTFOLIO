#include "pch.h"
#include "RedisToken.h"


LockFreeObjectPool<TokenObject, true> _token_pool;
