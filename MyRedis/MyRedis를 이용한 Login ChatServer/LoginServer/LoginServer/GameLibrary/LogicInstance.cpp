#include "pch.h"
#include "LogicInstance.h"
#include "LogUtils.h"
#include "SessionInstance.h"
#include "NetSerializeBuffer.h"
#include "NetSession.h"
#include "NetService.h"


// Session Lock 줄일 수 있는지 고민
// 
LogicInstance::~LogicInstance()
{

}

void LogicInstance::Disconnect(uint64 session_id)
{
	_service->Disconnect(session_id);
}
