#include "pch.h"
#include "LanSerializeBuffer.h"
#include "LanSendPacket.h"
#include "LanRecvPacket.h"
#include "LanClient.h"
#include "LanService.h"

void LanSerializeBuffer::AddCnt()
{
    if (_idx == 0)
    {
        AtomicIncrement32(&reinterpret_cast<LanSendPacket*>(reinterpret_cast<size_t>(this) - offsetof(LanSendPacket, _buffer))->_ref_cnt);
    }
    else
    {
        AtomicIncrement32(&reinterpret_cast<LanRecvPacket*>(reinterpret_cast<size_t>(this) - offsetof(LanRecvPacket, _buffers[_idx - 1]))->_ref_cnt);
    }
}

void LanSerializeBuffer::RemoveCnt()
{
    if (_idx == 0)
    {
        if (AtomicDecrement32(&reinterpret_cast<LanSendPacket*>(reinterpret_cast<size_t>(this) - offsetof(LanSendPacket, _buffer))->_ref_cnt) == 0)
        {
            LanSendPacket::g_send_lan_packet_pool.Free(reinterpret_cast<LanSendPacket*>(reinterpret_cast<size_t>(this) - offsetof(LanSendPacket, _buffer)));

        }
    }
    else
    {
        LanRecvPacket* recv_packet = reinterpret_cast<LanRecvPacket*>(reinterpret_cast<size_t>(this) - offsetof(LanRecvPacket, _buffers[_idx - 1]));

        if (AtomicDecrement32(&recv_packet->_ref_cnt) == 0)
        {
            LanRecvPacket::g_recv_lan_packet_pool.Free(recv_packet);
        }
    }
}

int32 LanSerializeBuffer::GetCnt()
{
    if (_idx == 0)
    {
        return *(reinterpret_cast<int*>((char*)this - sizeof(int32)));
    }
    else
    {
        return *(reinterpret_cast<int*>((char*)this - sizeof(LanSerializeBuffer) * (_idx - 1) - sizeof(int32)));
    }
}
