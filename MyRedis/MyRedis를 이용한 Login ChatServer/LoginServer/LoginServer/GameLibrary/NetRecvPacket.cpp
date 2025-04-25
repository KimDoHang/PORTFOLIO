#include "pch.h"
#include "NetRecvPacket.h"

LockFreeObjectPoolTLS<NetRecvPacket, false> NetRecvPacket::g_recv_net_packet_pool;

NetRecvPacket::NetRecvPacket(int iBufferSize)
{
    _recv_buffer = new char[iBufferSize];
    _recv_buffer_size = iBufferSize;
    // _recv_idx = _ref_cnt = _recv_writePos = _recv_readPos = 0;

    for (int i = 0; i < eNET_SERIALIZE_BUFFER_ARR_SIZE; i++)
    {
        _buffers[i]._idx = i + 1;
    }
}

NetRecvPacket::~NetRecvPacket()
{
    delete[] _recv_buffer;
}

void NetRecvPacket::Clear(void)
{
    _ref_cnt = 1;
    _recv_idx = _recv_writePos = _recv_readPos = 0;
}

