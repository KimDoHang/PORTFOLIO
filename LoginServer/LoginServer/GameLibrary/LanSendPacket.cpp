#include "pch.h"
#include "LanSendPacket.h"

LockFreeObjectPoolTLS<LanSendPacket, false> LanSendPacket::g_send_lan_packet_pool;

LanSendPacket::LanSendPacket(int iBufferSize) : _buffer(iBufferSize)
{
    _ref_cnt = 0;
    _buffer._idx = 0;
    _buffer._chpBuffer = new char[iBufferSize];
}

LanSendPacket::~LanSendPacket()
{
    delete[] _buffer._chpBuffer;
}

void LanSendPacket::LanClear()
{
    _ref_cnt = 1;
    _buffer.LanClear();
}
