#pragma once

#include "LanSerializeBuffer.h"
#include "LockFreeObjectPoolTLS.h"
#include "LockFreeObjectPool.h"
/*
    SendPacket의 경우 전달용으로 하나의 직렬화 버퍼만을 가진다.
    ALLOC_PACKET을 통해 할당이 가능하다.
    IDX = 0으로 RecvPacket인지 SendPacket인지를 구분한다. 이를 이용해 사용하는 BYTE 크기를 줄였다.
*/

class LanSendPacket
{
    friend class LanSerializeBuffer;
public:

    LanSendPacket(int iBufferSize = eLAN_SEND_BUFFER_DEFAULT);
    ~LanSendPacket();

    void LanClear();

    __forceinline static LanSerializeBuffer* LanAlloc()
    {
        LanSendPacket* packet = g_send_lan_packet_pool.Alloc();
        packet->LanClear();
        return &packet->_buffer;
    }

    __forceinline static void LanFree(LanSerializeBuffer* buffer)
    {
        if (AtomicDecrement32(&reinterpret_cast<LanSendPacket*>(reinterpret_cast<size_t>(buffer) - offsetof(LanSendPacket, _buffer))->_ref_cnt) == 0)
        {
            g_send_lan_packet_pool.Free(reinterpret_cast<LanSendPacket*>(reinterpret_cast<size_t>(buffer) - offsetof(LanSendPacket, _buffer)));
        }
    }

    __forceinline static void LanFree(LanSendPacket* buffer)
    {
        if (AtomicDecrement32(&buffer->_ref_cnt) == 0)
        {
            g_send_lan_packet_pool.Free(buffer);
        }
    }

    __forceinline static uint32 GetLANChunkCapacityCount()
    {
        return g_send_lan_packet_pool.GetChunkCapacityCount();
    }

    __forceinline static uint32 GetLANChunkUseCount()
    {
        return g_send_lan_packet_pool.GetChunkUseCount();
    }

    __forceinline static uint32 GetLANChunkReleaseCount()
    {
        return g_send_lan_packet_pool.GetChunkReleaseCount();
    }

    __forceinline static uint32 GetLANNodeUseCount()
    {
        return g_send_lan_packet_pool.GetChunkNodeUseCount();
    }

    __forceinline static uint32 GetLANNodeReleaseCount()
    {
        return g_send_lan_packet_pool.GetChunkNodeReleaseCount();
    }

    static LockFreeObjectPoolTLS<LanSendPacket, false> g_send_lan_packet_pool;

private:
    int32 _ref_cnt;
    LanSerializeBuffer _buffer;
};

#define ALLOC_LAN_PACKET() LanSendPacket::LanAlloc();
#define FREE_LAN_SEND_PACKET(buffer)  LanSendPacket::LanFree(buffer);


