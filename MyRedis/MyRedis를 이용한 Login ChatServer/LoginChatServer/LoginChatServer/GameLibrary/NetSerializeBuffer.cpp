#include "pch.h"
#include "NetSerializeBuffer.h"
#include "NetSendPacket.h"
#include "NetRecvPacket.h"
#include "LogUtils.h"

void NetSerializeBuffer::AddCnt()
{
    if (_idx == 0)
    {
        AtomicIncrement32(&reinterpret_cast<NetSendPacket*>(reinterpret_cast<size_t>(this) - offsetof(NetSendPacket, _buffer))->_ref_cnt);
    }
    else
    {
        AtomicIncrement32(&reinterpret_cast<NetRecvPacket*>(reinterpret_cast<size_t>(this) - offsetof(NetRecvPacket, _buffers[_idx - 1]))->_ref_cnt);
    }
}

void NetSerializeBuffer::RemoveCnt()
{
    if (_idx == 0)
    {
        if (AtomicDecrement32(&reinterpret_cast<NetSendPacket*>(reinterpret_cast<size_t>(this) - offsetof(NetSendPacket, _buffer))->_ref_cnt) == 0)
        {
            NetSendPacket::g_send_net_packet_pool.Free(reinterpret_cast<NetSendPacket*>(reinterpret_cast<size_t>(this) - offsetof(NetSendPacket, _buffer)));
        }
    }
    else
    {

        NetRecvPacket* recv_packet = reinterpret_cast<NetRecvPacket*>(reinterpret_cast<size_t>(this) 
            - offsetof(NetRecvPacket, _buffers[_idx - 1]));

        if (AtomicDecrement32(&recv_packet->_ref_cnt) == 0)
        {
            NetRecvPacket::g_recv_net_packet_pool.Free(recv_packet);
        }
    }
}

int32 NetSerializeBuffer::GetCnt()
{
    if (_idx == 0)
    {
        return *(reinterpret_cast<int*>((char*)this - sizeof(int32)));
    }
    else
    {
        return *(reinterpret_cast<int*>((char*)this - sizeof(NetSerializeBuffer) * (_idx - 1) - sizeof(int32)));
    }
}

void NetSerializeBuffer::ReSize()
{
    if (_idx != 0)
    {
        //RecvPacket�� ��� Resize�� �̷�������� �ȵȴ�.
        __debugbreak();
    }

    //�� ���� �Ҵ�
    int32 new_buffer_size = _iBUfferSize * 2 + 1;
    int32 past_buffer_size = _iBUfferSize;
    char* temp_buf = new char[new_buffer_size];

    //��ü ���� ũ��
    int32 data_size = GetDataSize();

    //header�� ������ ��ġ���� ��� ����
    memcpy(temp_buf + sizeof(NetHeader), (_chpBuffer + _readPos), data_size);
    delete[] _chpBuffer;

    //���� �� �����͵� ���Ҵ�
    _chpBuffer = temp_buf;
    _readPos = sizeof(NetHeader);
    _writePos = sizeof(NetHeader) + data_size;
    _iBUfferSize = new_buffer_size;

    g_logutils->PrintLibraryLog(NET_LOG_DIR, NET_FILE_NAME, L"SerializeBuffer Resize [%d -> %d]", past_buffer_size, new_buffer_size);
}

