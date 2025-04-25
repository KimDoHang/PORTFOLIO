#pragma once

#include "NetSerializeBuffer.h"
#include "LockFreeObjectPoolTLS.h"

/*
    SendPacket의 경우 전달용으로 하나의 직렬화 버퍼만을 가진다.
    ALLOC_PACKET을 통해 할당이 가능하다.
    IDX = 0으로 RecvPacket인지 SendPacket인지를 구분한다. 이를 이용해 사용하는 BYTE 크기를 줄였다.
*/

class NetSendPacket
{
    friend class NetSerializeBuffer;
public:

    NetSendPacket(int iBufferSize = eNET_SEND_BUFFER_DEFAULT);
    ~NetSendPacket();

    void NetClear();

    __forceinline static NetSerializeBuffer* NetAlloc()
    {
        NetSendPacket* packet = g_send_net_packet_pool.Alloc();
        packet->NetClear();
        return &packet->_buffer;
    }

    __forceinline static void NetFree(NetSerializeBuffer* buffer)
    {
        if (AtomicDecrement32(&reinterpret_cast<NetSendPacket*>(reinterpret_cast<size_t>(buffer) - offsetof(NetSendPacket, _buffer))->_ref_cnt) == 0)
        {
            NetSendPacket::g_send_net_packet_pool.Free(reinterpret_cast<NetSendPacket*>(reinterpret_cast<size_t>(buffer) - offsetof(NetSendPacket, _buffer)));
        }
    }


    //Net
    __forceinline static uint32 GetNETChunkCapacityCount()
    {
        return g_send_net_packet_pool.GetChunkCapacityCount();
    }

    __forceinline static uint32 GetNETChunkUseCount()
    {
        return g_send_net_packet_pool.GetChunkUseCount();
    }

    __forceinline static uint32 GetNETChunkReleaseCount()
    {
        return g_send_net_packet_pool.GetChunkReleaseCount();
    }

    __forceinline static uint32 GetNETNodeUseCount()
    {
        return g_send_net_packet_pool.GetChunkNodeUseCount();
    }

    __forceinline static uint32 GetNETNodeReleaseCount()
    {
        return g_send_net_packet_pool.GetChunkNodeReleaseCount();
    }

    static LockFreeObjectPoolTLS<NetSendPacket, false> g_send_net_packet_pool;

private:
    int32 _ref_cnt;
    NetSerializeBuffer _buffer;
};

#define ALLOC_NET_PACKET() NetSendPacket::NetAlloc();
#define FREE_NET_SEND_PACKET(buffer)  NetSendPacket::NetFree(buffer);

