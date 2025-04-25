#pragma once

#include "NetSerializeBuffer.h"
#include "LockFreeObjectPoolTLS.h"

#pragma warning(disable: 4244)

class NetRecvPacket
{
public:
    friend class NetSerializeBuffer;

    NetRecvPacket(int iBufferSize = eNET_RECV_BUFFER_DEFAULT);

    ~NetRecvPacket();

    /*---------------------------------------------------------------
    Packet Enum.

    ----------------------------------------------------------------*/

    NetSerializeBuffer* GetNetSerializeBuffer(int32 data_size)
    {
        _buffers[_recv_idx].NetClear();
        _buffers[_recv_idx]._chpBuffer = _recv_buffer + _recv_readPos;
        _buffers[_recv_idx]._writePos += data_size;
        _buffers[_recv_idx]._iBUfferSize = _buffers[_recv_idx]._writePos;
        return &_buffers[_recv_idx];
    }

    void Clear();

    __forceinline void MoveWritePos(int iSize)
    {
        if (iSize > GetFreeSize())
            __debugbreak();

        _recv_writePos += iSize;
    }

    __forceinline bool MoveIdxAndPos(int iSize)
    {
        _recv_readPos += iSize;

        if ((++_recv_idx) == eNET_SERIALIZE_BUFFER_ARR_SIZE)
            return false;

        return true;
    }

    __forceinline bool Used()
    {
        if (_recv_readPos != 0)
            return true;

        return false;
    }

    __forceinline void CopyRemainData(NetRecvPacket* recv_packet)
    {

        if (recv_packet->GetDataSize() == 0)
            return;

        memcpy(_recv_buffer + _recv_writePos, recv_packet->GetReadPos(), recv_packet->GetDataSize());
        _recv_writePos += recv_packet->GetDataSize();
    }

    __forceinline  char* GetReadPos() { return _recv_buffer + _recv_readPos; }
    __forceinline char* GetWritePos() { return _recv_buffer + _recv_writePos; }

    __forceinline int32  GetDataSize() { return _recv_writePos - _recv_readPos; }
    __forceinline int32  GetFreeSize() { return _recv_buffer_size - _recv_writePos; }

    __forceinline int32 GetBufferSize() { return _recv_buffer_size; }


    __forceinline static NetRecvPacket* NetAlloc()
    {
        NetRecvPacket* recv_packet = g_recv_net_packet_pool.Alloc();
        recv_packet->Clear();
        return recv_packet;
    }

    __forceinline static void NetFree(NetRecvPacket* recv_packet)
    {
        if (AtomicDecrement32(&recv_packet->_ref_cnt) == 0)
            g_recv_net_packet_pool.Free(recv_packet);
    }

    __forceinline static void NetFree(NetSerializeBuffer* packet)
    {
        NetRecvPacket* recv_packet = reinterpret_cast<NetRecvPacket*>(reinterpret_cast<size_t>(packet) - offsetof(NetRecvPacket, _buffers[packet->_idx - 1]));
        if (AtomicDecrement32(&recv_packet->_ref_cnt) == 0)
            g_recv_net_packet_pool.Free(recv_packet);
    }

    __forceinline static uint32 GetNetChunkCapacityCount()
    {
        return g_recv_net_packet_pool.GetChunkCapacityCount();
    }

    __forceinline static uint32 GetNetChunkUseCount()
    {
        return g_recv_net_packet_pool.GetChunkUseCount();
    }

    __forceinline static uint32 GetNetChunkReleaseCount()
    {
        return g_recv_net_packet_pool.GetChunkReleaseCount();
    }

    __forceinline static uint32 GetNetNodeUseCount()
    {
        return g_recv_net_packet_pool.GetChunkNodeUseCount();
    }

    __forceinline static uint32 GetNetNodeReleaseCount()
    {
        return g_recv_net_packet_pool.GetChunkNodeReleaseCount();
    }

    static LockFreeObjectPoolTLS<NetRecvPacket, false> g_recv_net_packet_pool;

private:
    char* _recv_buffer;
    uint8 _recv_idx;
    int32 _recv_buffer_size;
    int32 _recv_readPos;
    int32 _recv_writePos;
    int32 _ref_cnt;
    NetSerializeBuffer _buffers[eNET_SERIALIZE_BUFFER_ARR_SIZE];
};

///Alloc시에 Ref_Cnt 1인 상태로 반환
#define ALLOC_NET_RECV_PACKET() NetRecvPacket::NetAlloc()
///Free시에 Ref_Cnt 1 감소
#define FREE_NET_RECV_PACKET(packet) NetRecvPacket::NetFree(packet)

#pragma warning(default: 4244)
