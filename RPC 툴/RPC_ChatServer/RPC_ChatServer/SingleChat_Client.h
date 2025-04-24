#pragma once
#include "NetSerializeBuffer.h"
#include "NetClient.h"

class SingleChat_Client : public NetClient
{
public:
    virtual void OnConnect();
    virtual void OnRelease();
    virtual void OnRecvMsg(NetSerializeBuffer * msg);
    virtual void OnError(const int8 log_level, int32 err_code, const WCHAR * cause);
    virtual void OnExit();

public:
    void Marshal_ChatReq_Login(WCHAR* id, WCHAR* nick_name, char* session_key);
    void Marshal_ChatReq_SectorMove(int64 account_num, uint16 sector_x, uint16 sector_y);
    void Marshal_ChatReq_Message(int64 account_num, uint16 message_len, WCHAR* message);
    void Marshal_ChatReq_HeartBeat();
    void UnMarshal_ChatRes_Login(NetSerializeBuffer * msg);
    void UnMarshal_ChatRes_SectorMove(NetSerializeBuffer * msg);
    void UnMarshal_ChatRes_Message(NetSerializeBuffer * msg);
    void Handle_ChatRes_Login(uint8 status, int64 account_num);
    void Handle_ChatRes_SectorMove(int64 account_num, uint16 sector_x, uint16 sector_y);
    void Handle_ChatRes_Message(int64 account_num, WCHAR* id, WCHAR* nick_name, uint16 message_len, WCHAR* message);

};