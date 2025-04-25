#pragma once

#include "LanSerializeBuffer.h"
#include "LockFreeObjectPoolTLS.h"
#include "LockFreeObjectPool.h"
/*
    SendPacket�� ��� ���޿����� �ϳ��� ����ȭ ���۸��� ������.
    ALLOC_PACKET�� ���� �Ҵ��� �����ϴ�.
    IDX = 0���� RecvPacket���� SendPacket������ �����Ѵ�. �̸� �̿��� ����ϴ� BYTE ũ�⸦ �ٿ���.
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


