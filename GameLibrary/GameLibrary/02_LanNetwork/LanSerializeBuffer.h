#pragma once
#include "LogUtils.h"
#pragma warning(disable: 4244)


#pragma pack(push,1)
class LanHeader
{
public:
    friend class LanService;
    friend class LanClient;

private:
    uint16 _len;
};
#pragma pack(pop)

class LanMsgException : public std::exception
{
public:
    // ������ (LanMsgException �Է�)
    explicit LanMsgException(const std::wstring& msg) : wmessage(msg) {}

    // WCHAR* ��ȯ �Լ�
    const WCHAR* whatW() const noexcept {
        return wmessage.c_str();
    }
private:
    std::wstring wmessage;  // ���� �޽��� (�����ڵ�)
};

class LanSerializeBuffer
{
public:
    friend class LanClient;
    friend class LanService;
    friend class LanRecvPacket;
    friend class LanSendPacket;
    /*---------------------------------------------------------------
    Packet Enum.

    ----------------------------------------------------------------*/

    static const int32 eLAN_BUFFER_DEFAULT = 512;

    //////////////////////////////////////////////////////////////////////////
    // ������, �ı���.
    //
    // Return:
    //////////////////////////////////////////////////////////////////////////
    __inline LanSerializeBuffer();
    __inline LanSerializeBuffer(int iBufferSize);

    __inline virtual	~LanSerializeBuffer();


    //////////////////////////////////////////////////////////////////////////
    // ��Ŷ û��.
    //
    // Parameters: ����.
    // Return: ����.
    //////////////////////////////////////////////////////////////////////////

    __forceinline void LanClear()
    {
        _readPos = _writePos = sizeof(LanHeader);
    }

    //////////////////////////////////////////////////////////////////////////
    // ���� ������ ���.
    //
    // Parameters: ����.
    // Return: (int)��Ŷ ���� ������ ���.
    //////////////////////////////////////////////////////////////////////////
    __inline int	GetBufferSize(void) { return _iBUfferSize; }
    //////////////////////////////////////////////////////////////////////////
    // ���� ������� ������ ���.
    //
    // Parameters: ����.
    // Return: (int)������� ����Ÿ ������.
    //////////////////////////////////////////////////////////////////////////
    __inline int		GetDataSize(void) const { return _writePos - _readPos; }
    __inline int		GetFreeSize(void) const { return _iBUfferSize - _writePos; }


    //////////////////////////////////////////////////////////////////////////
    // ���� ������ ���.
    //
    // Parameters: ����.
    // Return: (char *)���� ������.
    //////////////////////////////////////////////////////////////////////////
    __inline char* GetBufferPtr(void) { return _chpBuffer + sizeof(LanHeader); }
    __inline char* GetReadPtr(void) { return _chpBuffer + _readPos; }
    //////////////////////////////////////////////////////////////////////////
    // ���� Pos �̵�. (�����̵��� �ȵ�)
    // GetBufferPtr �Լ��� �̿��Ͽ� �ܺο��� ������ ���� ������ ������ ��� ���. 
    //
    // Parameters: (int) �̵� ������.
    // Return: (int) �̵��� ������.
    //////////////////////////////////////////////////////////////////////////
    __inline int		MoveWritePos(int iSize);
    __inline int		MoveReadPos(int iSize);



    /* ============================================================================= */
    // ������ �����ε�
    /* ============================================================================= */

    //////////////////////////////////////////////////////////////////////////
    // �ֱ�.	�� ���� Ÿ�Ը��� ��� ����.
    //////////////////////////////////////////////////////////////////////////
    __forceinline LanSerializeBuffer& operator << (unsigned char byValue);
    __forceinline LanSerializeBuffer& operator << (char chValue);

    __forceinline LanSerializeBuffer& operator << (short shValue);
    __forceinline LanSerializeBuffer& operator << (unsigned short wValue);

    __forceinline LanSerializeBuffer& operator << (int iValue);
    __forceinline LanSerializeBuffer& operator << (long lValue);
    __forceinline LanSerializeBuffer& operator << (unsigned int dwValue);
    __forceinline LanSerializeBuffer& operator << (float fValue);

    __forceinline LanSerializeBuffer& operator << (__int64 iValue);
    __forceinline LanSerializeBuffer& operator << (unsigned __int64 iValue);

    __forceinline LanSerializeBuffer& operator << (double dValue);


    //////////////////////////////////////////////////////////////////////////
    // ����.	�� ���� Ÿ�Ը��� ��� ����.
    //////////////////////////////////////////////////////////////////////////
    __forceinline LanSerializeBuffer& operator >> (unsigned char& byValue);
    __forceinline LanSerializeBuffer& operator >> (char& chValue);

    __forceinline LanSerializeBuffer& operator >> (short& shValue);
    __forceinline LanSerializeBuffer& operator >> (unsigned short& wValue);

    __forceinline LanSerializeBuffer& operator >> (int& iValue);
    __forceinline LanSerializeBuffer& operator >> (long& lValue);
    __forceinline LanSerializeBuffer& operator >> (unsigned int& dwValue);
    __forceinline LanSerializeBuffer& operator >> (float& fValue);

    __forceinline LanSerializeBuffer& operator >> (__int64& iValue);
    __forceinline LanSerializeBuffer& operator >> (unsigned __int64& iValue);

    __forceinline LanSerializeBuffer& operator >> (double& dValue);

    //////////////////////////////////////////////////////////////////////////
    // ����Ÿ ���.
    //
    // Parameters: (char *)Dest ������. (int)Size.
    // Return: (int)������ ������.
    //////////////////////////////////////////////////////////////////////////
    __inline int		GetData(char* chpDest, int iSize);

    //////////////////////////////////////////////////////////////////////////
    // ����Ÿ ����.
    //
    // Parameters: (char *)Src ������. (int)SrcSize.
    // Return: (int)������ ������.
    //////////////////////////////////////////////////////////////////////////
    __inline int		PutData(const char* chpSrc, int iSrcSize);

    __inline void       ReSize();

public:
    //RefCounter
    void AddCnt();
    void RemoveCnt();
    int32 GetCnt();

private:
    int GetPacketSize() { return _writePos; }
    char* GetPacketPtr() { return _chpBuffer; }
protected:
    char* _chpBuffer;
    int _readPos;
    int _writePos;
    int	_iBUfferSize;
    int8 _encryption;
    int8 _send_disconnect_flag;
    uint8 _idx;
};
//////////////////////////////////////////////////////////////////////////

LanSerializeBuffer::LanSerializeBuffer()
{
    _chpBuffer = nullptr;
    _iBUfferSize = -1;
    _readPos = _writePos = 0;
    _encryption = 0;
}

LanSerializeBuffer::LanSerializeBuffer(int iBufferSize)
{
    _chpBuffer = nullptr;
    _iBUfferSize = iBufferSize;
    _readPos = _writePos = 0;
    _encryption = 0;
}

LanSerializeBuffer::~LanSerializeBuffer()
{
    //delete[] _chpBuffer;
}

//�����ϰ� �������̹Ƿ� 
int LanSerializeBuffer::MoveWritePos(int iSize)
{
    if (GetFreeSize() < iSize)
        __debugbreak();

    _writePos += iSize;
    return iSize;
}

int LanSerializeBuffer::MoveReadPos(int iSize)
{
    if (GetDataSize() < iSize)
        __debugbreak();

    _readPos += iSize;
    return iSize;
}

LanSerializeBuffer& LanSerializeBuffer::operator<<(unsigned char byValue)
{
    while (GetFreeSize() < sizeof(unsigned char))
    {
        ReSize();
    }

    *(unsigned char*)(_chpBuffer + _writePos) = byValue;
    _writePos += sizeof(byValue);
    return *this;
}

LanSerializeBuffer& LanSerializeBuffer::operator<<(char chValue)
{
    while (GetFreeSize() < sizeof(char))
    {
        ReSize();
    }

    *(char*)(_chpBuffer + _writePos) = chValue;
    _writePos += sizeof(chValue);
    return *this;
}

LanSerializeBuffer& LanSerializeBuffer::operator<<(short shValue)
{
    while (GetFreeSize() < sizeof(short))
    {
        ReSize();
    }

    *(short*)(_chpBuffer + _writePos) = shValue;
    _writePos += sizeof(shValue);
    return *this;

}

LanSerializeBuffer& LanSerializeBuffer::operator<<(unsigned short wValue)
{
    if (GetFreeSize() < sizeof(unsigned short))
    {
        ReSize();
    }

    *(unsigned short*)(_chpBuffer + _writePos) = wValue;
    _writePos += sizeof(wValue);

    return *this;
}

LanSerializeBuffer& LanSerializeBuffer::operator<<(int iValue)
{
    while (GetFreeSize() < sizeof(int))
    {
        ReSize();
    }

    *(int*)(_chpBuffer + _writePos) = iValue;
    _writePos += sizeof(iValue);
    return *this;
}

LanSerializeBuffer& LanSerializeBuffer::operator<<(long lValue)
{
    while (GetFreeSize() < sizeof(long))
    {
        ReSize();

    }
    *(long*)(_chpBuffer + _writePos) = lValue;
    _writePos += sizeof(lValue);
    return *this;
}

inline LanSerializeBuffer& LanSerializeBuffer::operator<<(unsigned int dwValue)
{
    while (GetFreeSize() < sizeof(unsigned int))
    {
        ReSize();
    }

    *(unsigned int*)(_chpBuffer + _writePos) = dwValue;
    _writePos += sizeof(dwValue);
    return *this;

}

LanSerializeBuffer& LanSerializeBuffer::operator<<(float fValue)
{
    while (GetFreeSize() < sizeof(float))
    {
        ReSize();
    }

    *(float*)(_chpBuffer + _writePos) = fValue;
    _writePos += sizeof(fValue);
    return *this;

}

LanSerializeBuffer& LanSerializeBuffer::operator<<(__int64 iValue)
{
    while (GetFreeSize() < sizeof(__int64))
    {
        ReSize();
    }

    *(__int64*)(_chpBuffer + _writePos) = iValue;
    _writePos += sizeof(iValue);
    return *this;
}

inline LanSerializeBuffer& LanSerializeBuffer::operator<<(unsigned __int64 iValue)
{
    while (GetFreeSize() < sizeof(unsigned __int64))
    {
        ReSize();
    }

    *(unsigned __int64*)(_chpBuffer + _writePos) = iValue;
    _writePos += sizeof(iValue);
    return *this;
}

LanSerializeBuffer& LanSerializeBuffer::operator<<(double dValue)
{
    while (GetFreeSize() < sizeof(double))
    {
        ReSize();
    }

    *(double*)(_chpBuffer + _writePos) = dValue;
    _writePos += sizeof(dValue);
    return *this;
}

inline LanSerializeBuffer& LanSerializeBuffer::operator>>(unsigned char& byValue)
{
    if (GetDataSize() < sizeof(unsigned char))
    {
        throw LanMsgException(L"operator >> unsigned char");
    }

    byValue = *(unsigned char*)(_chpBuffer + _readPos);
    _readPos += sizeof(byValue);
    return *this;
}

LanSerializeBuffer& LanSerializeBuffer::operator>>(char& chValue)
{
    if (GetDataSize() < sizeof(char))
    {
        throw LanMsgException(L"operator >> char");
    }

    chValue = *(char*)(_chpBuffer + _readPos);
    _readPos += sizeof(chValue);
    return *this;
}

LanSerializeBuffer& LanSerializeBuffer::operator>>(short& shValue)
{
    if (GetDataSize() < sizeof(short))
    {
        throw LanMsgException(L"operator >> short");
    }

    shValue = *(short*)(_chpBuffer + _readPos);
    _readPos += sizeof(shValue);
    return *this;
}

inline LanSerializeBuffer& LanSerializeBuffer::operator>>(unsigned short& wValue)
{
    if (GetDataSize() < sizeof(unsigned short))
    {
        throw LanMsgException(L"operator >> unsigned char");
    }

    wValue = *(unsigned short*)(_chpBuffer + _readPos);
    _readPos += sizeof(wValue);
    return *this;
}

LanSerializeBuffer& LanSerializeBuffer::operator>>(int& iValue)
{
    if (GetDataSize() < sizeof(int))
    {
        throw LanMsgException(L"operator >> int");
    }

    iValue = *(int*)(_chpBuffer + _readPos);
    _readPos += sizeof(iValue);
    return *this;
}
inline LanSerializeBuffer& LanSerializeBuffer::operator>>(long& lValue)
{
    if (GetDataSize() < sizeof(long))
    {
        throw LanMsgException(L"operator >> long");
    }

    lValue = *(long*)(_chpBuffer + _readPos);
    _readPos += sizeof(lValue);
    return *this;
}

inline LanSerializeBuffer& LanSerializeBuffer::operator>>(unsigned int& dwValue)
{
    if (GetDataSize() < sizeof(unsigned int))
    {
        throw LanMsgException(L"operator >> unsigned int");
    }

    dwValue = *(unsigned int*)(_chpBuffer + _readPos);
    _readPos += sizeof(dwValue);
    return *this;
}

LanSerializeBuffer& LanSerializeBuffer::operator>>(float& fValue)
{
    if (GetDataSize() < sizeof(float))
    {
        throw LanMsgException(L"operator >> float");
    }
    fValue = *(float*)(_chpBuffer + _readPos);
    _readPos += sizeof(fValue);
    return *this;
}

LanSerializeBuffer& LanSerializeBuffer::operator>>(__int64& iValue)
{
    if (GetDataSize() < sizeof(__int64))
    {
        throw LanMsgException(L"operator >> __int64");
    }

    iValue = *(__int64*)(_chpBuffer + _readPos);
    _readPos += sizeof(iValue);
    return *this;
}

inline LanSerializeBuffer& LanSerializeBuffer::operator>>(unsigned __int64& iValue)
{
    if (GetDataSize() < sizeof(unsigned __int64))
    {
        throw LanMsgException(L"operator >> unsigned __int64");
    }

    iValue = *(unsigned __int64*)(_chpBuffer + _readPos);
    _readPos += sizeof(iValue);
    return *this;
}

LanSerializeBuffer& LanSerializeBuffer::operator>>(double& dValue)
{
    if (GetDataSize() < sizeof(double))
    {
        throw LanMsgException(L"operator >> double");
    }

    dValue = *(double*)(_chpBuffer + _readPos);
    _readPos += sizeof(dValue);
    return *this;

}

int LanSerializeBuffer::GetData(char* chpDest, int copy_size)
{
    if (GetDataSize() < copy_size)
    {
        WCHAR err_msg[50];
        wsprintf(err_msg, L"operator >> GetData copy_size:%d", copy_size);
        throw LanMsgException(err_msg);
    }

    memcpy(chpDest, (_chpBuffer + _readPos), copy_size);
    _readPos += copy_size;

    return copy_size;
}

int LanSerializeBuffer::PutData(const char* chpSrc, int copy_size)
{
    while (GetFreeSize() < copy_size)
    {
        ReSize();
    }

    memcpy((_chpBuffer + _writePos), chpSrc, copy_size);
    _writePos += copy_size;

    return copy_size;
}

inline void LanSerializeBuffer::ReSize()
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
    memcpy(temp_buf + sizeof(LanHeader), (_chpBuffer + _readPos), data_size);
    delete[] _chpBuffer;

    //���� �� �����͵� ���Ҵ�
    _chpBuffer = temp_buf;
    _readPos = sizeof(LanHeader);
    _writePos = sizeof(LanHeader) + data_size;
    _iBUfferSize = new_buffer_size;

    g_logutils->PrintLibraryLog(NET_LOG_DIR, NET_FILE_NAME, L"SerializeBuffer Resize [%d -> %d]", past_buffer_size, new_buffer_size);
}

#define LAN_PACKET_FREE(packet) packet->RemoveCnt()

#pragma warning(default: 4244)
