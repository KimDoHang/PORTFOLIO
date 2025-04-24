#pragma once

#include "LanSerializeBuffer.h"
#include "LockFreeObjectPoolTLS.h"
#include "LockFreeObjectPool.h"
#pragma warning(disable: 4244)

class LanRecvPacket
{
public:
    friend class LanSerializeBuffer;

    LanRecvPacket(int iBufferSize = eLAN_RECV_BUFFER_DEFAULT);

    ~LanRecvPacket();

    /*---------------------------------------------------------------
    Packet Enum.

    ----------------------------------------------------------------*/

    LanSerializeBuffer* GetLanSerializeBuffer(int32 data_size)
    {
        _buffers[_recv_idx].LanClear();
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

        if ((++_recv_idx) == eLAN_SERIALIZE_BUFFER_ARR_SIZE)
            return false;

        return true;
    }

    bool Used()
    {
        if (_recv_readPos != 0)
            return true;

        return false;
    }

    void CopyRemainData(LanRecvPacket* recv_packet)
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

    __forceinline static LanRecvPacket* LanAlloc()
    {
        LanRecvPacket* recv_packet = g_recv_lan_packet_pool.Alloc();
        recv_packet->Clear();
        return recv_packet;
    }

    __forceinline static void LanFree(LanRecvPacket* recv_packet)
    {
        if (AtomicDecrement32(&recv_packet->_ref_cnt) == 0)
            g_recv_lan_packet_pool.Free(recv_packet);
    }

    __forceinline static uint32 GetLanChunkCapacityCount()
    {
        return g_recv_lan_packet_pool.GetChunkCapacityCount();
    }

    __forceinline static uint32 GetLanChunkUseCount()
    {
        return g_recv_lan_packet_pool.GetChunkUseCount();
    }

    __forceinline static uint32 GetLanChunkReleaseCount()
    {
        return g_recv_lan_packet_pool.GetChunkReleaseCount();
    }

    __forceinline static uint32 GetLanNodeUseCount()
    {
        return g_recv_lan_packet_pool.GetChunkNodeUseCount();
    }

    __forceinline static uint32 GetLanNodeReleaseCount()
    {
        return g_recv_lan_packet_pool.GetChunkNodeReleaseCount();
    }

    static LockFreeObjectPoolTLS<LanRecvPacket, false> g_recv_lan_packet_pool;

private:
    char* _recv_buffer;
    uint8 _recv_idx;
    int32 _recv_buffer_size;
    int32 _recv_readPos;
    int32 _recv_writePos;
    int32 _ref_cnt;
    LanSerializeBuffer _buffers[eLAN_SERIALIZE_BUFFER_ARR_SIZE];
};

///Alloc시에 Ref_Cnt 1인 상태로 반환
#define ALLOC_LAN_RECV_PACKET() LanRecvPacket::LanAlloc()
#define FREE_LAN_RECV_PACKET(packet) LanRecvPacket::LanFree(packet)


#pragma warning(default: 4244)
