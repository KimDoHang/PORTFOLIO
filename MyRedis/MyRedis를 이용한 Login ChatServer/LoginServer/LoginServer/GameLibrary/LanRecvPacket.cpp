#include "pch.h"
#include "LanRecvPacket.h"

LockFreeObjectPoolTLS<LanRecvPacket, false> LanRecvPacket::g_recv_lan_packet_pool;

LanRecvPacket::LanRecvPacket(int iBufferSize)
{
    _recv_buffer = new char[iBufferSize];
    _recv_buffer_size = iBufferSize;

    for (int i = 0; i < eLAN_SERIALIZE_BUFFER_ARR_SIZE; i++)
    {
        _buffers[i]._idx = i + 1;
    }
}

LanRecvPacket::~LanRecvPacket()
{
    delete[] _recv_buffer;
}

void LanRecvPacket::Clear(void)
{
    _ref_cnt = 1;
    _recv_idx = _recv_writePos = _recv_readPos = 0;
}

