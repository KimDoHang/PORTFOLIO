#include "pch.h"
#include "NetSendPacket.h"

LockFreeObjectPoolTLS<NetSendPacket, false> NetSendPacket::g_send_net_packet_pool;

NetSendPacket::NetSendPacket(int iBufferSize) : _buffer(iBufferSize)
{
    _ref_cnt = 0;
    _buffer._idx = 0;
    _buffer._chpBuffer = new char[iBufferSize];
}

NetSendPacket::~NetSendPacket()
{
    delete[] _buffer._chpBuffer;
}


void NetSendPacket::NetClear()
{
    _ref_cnt = 1;
    _buffer.NetClear();
}

