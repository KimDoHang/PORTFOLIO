#pragma once
class SessionInstance
{
public:
	uint64 GetSessionID() { return _session_id; }
protected:
	uint64 _session_id;
};

