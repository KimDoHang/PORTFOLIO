#pragma once
#pragma warning(disable: 4244)


const int32 NET_EXCEPTION_ARR_SIZE = 128;

#pragma pack(push,1)
class NetHeader
{
public:
    friend class NetService;
    friend class NetClient;
private:
    uint8 _code;
    uint16 _len;
    uint8 _rand_seed;
    uint8 _check_sum;

};
#pragma pack(pop)


class NetMsgException : public std::exception 
{
public:
    // 생성자 (wstring 입력)
    explicit NetMsgException(const std::wstring& msg) : wmessage(msg) {}

    // WCHAR* 반환 함수
    const WCHAR* whatW() const noexcept {
        return wmessage.c_str();
    }
private:
    std::wstring wmessage;  // 예외 메시지 (유니코드)
};


#ifndef  __PACKET__
#define  __PACKET__

class NetSerializeBuffer
{
public:
    friend class NetService;
    friend class NetClient;

    friend class NetRecvPacket;
    friend class NetSendPacket;
    /*---------------------------------------------------------------
    Packet Enum.

    ----------------------------------------------------------------*/

    //////////////////////////////////////////////////////////////////////////
    // 생성자, 파괴자.
    //
    // Return:
    //////////////////////////////////////////////////////////////////////////
#pragma warning (disable : 26495)

    NetSerializeBuffer()
    {
        _chpBuffer = nullptr;
        _iBUfferSize = -1;
        _readPos = _writePos = 0;
        _encryption = 0;
    }

    NetSerializeBuffer(int iBufferSize)
    {
        _chpBuffer = nullptr;
        _iBUfferSize = iBufferSize;
        _readPos = _writePos = 0;
        _encryption = 0;
    }

#pragma warning (default : 26495)


    __inline virtual	~NetSerializeBuffer();


    //////////////////////////////////////////////////////////////////////////
    // 패킷 청소.
    //
    // Parameters: 없음.
    // Return: 없음.
    //////////////////////////////////////////////////////////////////////////
    //__forceinline void Clear(int16 packet_header_size)
    //{
    //    _readPos = _writePos = sizeof(packet_header_size);
    //    _encryption = 0;
    //}

    __forceinline void NetClear()
    {
        _readPos = _writePos = sizeof(NetHeader);
        _encryption = 0;
        _send_disconnect_flag = 0;
    }

    //////////////////////////////////////////////////////////////////////////
    // 버퍼 사이즈 얻기.
    //
    // Parameters: 없음.
    // Return: (int)패킷 버퍼 사이즈 얻기.
    //////////////////////////////////////////////////////////////////////////
    __inline int	GetBufferSize(void) { return _iBUfferSize; }
    //////////////////////////////////////////////////////////////////////////
    // 현재 사용중인 사이즈 얻기.
    //
    // Parameters: 없음.
    // Return: (int)사용중인 데이타 사이즈.
    //////////////////////////////////////////////////////////////////////////
    __inline int		GetDataSize(void) const { return _writePos - _readPos; }
    __inline int		GetFreeSize(void) const { return _iBUfferSize - _writePos; }


    //////////////////////////////////////////////////////////////////////////
    // 버퍼 포인터 얻기.
    //
    // Parameters: 없음.
    // Return: (char *)버퍼 포인터.
    //////////////////////////////////////////////////////////////////////////
    __inline char* GetBufferPtr(void) { return _chpBuffer + sizeof(NetHeader); }
    __inline char* GetReadPtr(void) { return _chpBuffer + _readPos; }
    //////////////////////////////////////////////////////////////////////////
    // 버퍼 Pos 이동. (음수이동은 안됨)
    // GetBufferPtr 함수를 이용하여 외부에서 강제로 버퍼 내용을 수정할 경우 사용. 
    //
    // Parameters: (int) 이동 사이즈.
    // Return: (int) 이동된 사이즈.
    //////////////////////////////////////////////////////////////////////////
    __inline int		MoveWritePos(int iSize);
    __inline int		MoveReadPos(int iSize);

    /* ============================================================================= */
    // 연산자 오버로딩
    /* ============================================================================= */

    //__forceinline NetSerializeBuffer& operator = (const NetSerializeBuffer& clSrcPacket);

    //////////////////////////////////////////////////////////////////////////
    // 넣기.	각 변수 타입마다 모두 만듬.
    //////////////////////////////////////////////////////////////////////////
    __forceinline NetSerializeBuffer& operator << (unsigned char byValue);
    __forceinline NetSerializeBuffer& operator << (char chValue);

    __forceinline NetSerializeBuffer& operator << (short shValue);
    __forceinline NetSerializeBuffer& operator << (unsigned short wValue);

    __forceinline NetSerializeBuffer& operator << (int iValue);
    __forceinline NetSerializeBuffer& operator << (long lValue);
    __forceinline NetSerializeBuffer& operator << (unsigned int dwValue);
    __forceinline NetSerializeBuffer& operator << (float fValue);

    __forceinline NetSerializeBuffer& operator << (__int64 iValue);
    __forceinline NetSerializeBuffer& operator << (unsigned __int64 iValue);

    __forceinline NetSerializeBuffer& operator << (double dValue);


    //////////////////////////////////////////////////////////////////////////
    // 빼기.	각 변수 타입마다 모두 만듬.
    //////////////////////////////////////////////////////////////////////////
    __forceinline NetSerializeBuffer& operator >> (unsigned char& byValue);
    __forceinline NetSerializeBuffer& operator >> (char& chValue);

    __forceinline NetSerializeBuffer& operator >> (short& shValue);
    __forceinline NetSerializeBuffer& operator >> (unsigned short& wValue);

    __forceinline NetSerializeBuffer& operator >> (int& iValue);
    __forceinline NetSerializeBuffer& operator >> (long& lValue);
    __forceinline NetSerializeBuffer& operator >> (unsigned int& dwValue);
    __forceinline NetSerializeBuffer& operator >> (float& fValue);

    __forceinline NetSerializeBuffer& operator >> (__int64& iValue);
    __forceinline NetSerializeBuffer& operator >> (unsigned __int64& iValue);

    __forceinline NetSerializeBuffer& operator >> (double& dValue);

    //////////////////////////////////////////////////////////////////////////
    // 데이타 얻기.
    //
    // Parameters: (char *)Dest 포인터. (int)Size.
    // Return: (int)복사한 사이즈.
    //////////////////////////////////////////////////////////////////////////
    __inline int		GetData(char* chpDest, int iSize);

    //////////////////////////////////////////////////////////////////////////
    // 데이타 삽입.
    //
    // Parameters: (char *)Src 포인터. (int)SrcSize.
    // Return: (int)복사한 사이즈.
    //////////////////////////////////////////////////////////////////////////
    __inline int		PutData(char* chpSrc, int iSrcSize);

    void       ReSize();

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




NetSerializeBuffer::~NetSerializeBuffer()
{
}

//저장하고 버릴것이므로 
int NetSerializeBuffer::MoveWritePos(int iSize)
{
    if (GetFreeSize() < iSize)
        __debugbreak();

    _writePos += iSize;

    return iSize;
}

int NetSerializeBuffer::MoveReadPos(int iSize)
{
    if (GetDataSize() < iSize)
        __debugbreak();

    _readPos += iSize;

    return iSize;
}

NetSerializeBuffer& NetSerializeBuffer::operator<<(unsigned char byValue)
{
    while (GetFreeSize() < sizeof(unsigned char))
    {
        ReSize();
    }

    *(unsigned char*)(_chpBuffer + _writePos) = byValue;
    _writePos += sizeof(byValue);
    return *this;
}

NetSerializeBuffer& NetSerializeBuffer::operator<<(char chValue)
{
    while (GetFreeSize() < sizeof(char))
    {
        ReSize();
    }

    *(char*)(_chpBuffer + _writePos) = chValue;
    _writePos += sizeof(chValue);

    return *this;
}

NetSerializeBuffer& NetSerializeBuffer::operator<<(short shValue)
{
    while (GetFreeSize() < sizeof(short))
    {
        ReSize();
    }

    *(short*)(_chpBuffer + _writePos) = shValue;
    _writePos += sizeof(shValue);
    return *this;

}

NetSerializeBuffer& NetSerializeBuffer::operator<<(unsigned short wValue)
{
    while (GetFreeSize() < sizeof(unsigned short))
    {
        ReSize();
    }

    *(unsigned short*)(_chpBuffer + _writePos) = wValue;
    _writePos += sizeof(wValue);

    return *this;
}

NetSerializeBuffer& NetSerializeBuffer::operator<<(int iValue)
{
    while (GetFreeSize() < sizeof(int))
    {
        ReSize();
    }


    *(int*)(_chpBuffer + _writePos) = iValue;
    _writePos += sizeof(iValue);
    return *this;
}

NetSerializeBuffer& NetSerializeBuffer::operator<<(long lValue)
{
    while (GetFreeSize() < sizeof(long))
    {
        ReSize();
    }

    *(long*)(_chpBuffer + _writePos) = lValue;
    _writePos += sizeof(lValue);
    return *this;
}

inline NetSerializeBuffer& NetSerializeBuffer::operator<<(unsigned int dwValue)
{
    while (GetFreeSize() < sizeof(unsigned int))
    {
        ReSize();
    }

    *(unsigned int*)(_chpBuffer + _writePos) = dwValue;
    _writePos += sizeof(dwValue);
    return *this;

}

NetSerializeBuffer& NetSerializeBuffer::operator<<(float fValue)
{
    while (GetFreeSize() < sizeof(float))
    {
        ReSize();
    }

    *(float*)(_chpBuffer + _writePos) = fValue;
    _writePos += sizeof(fValue);
    return *this;

}

NetSerializeBuffer& NetSerializeBuffer::operator<<(__int64 iValue)
{
    while (GetFreeSize() < sizeof(__int64))
    {
        ReSize();
    }

    *(__int64*)(_chpBuffer + _writePos) = iValue;
    _writePos += sizeof(iValue);
    return *this;
}

inline NetSerializeBuffer& NetSerializeBuffer::operator<<(unsigned __int64 iValue)
{
    while (GetFreeSize() < sizeof(unsigned __int64))
    {
        ReSize();
    }

    *(unsigned __int64*)(_chpBuffer + _writePos) = iValue;
    _writePos += sizeof(iValue);
    return *this;
}

NetSerializeBuffer& NetSerializeBuffer::operator<<(double dValue)
{
    while (GetFreeSize() < sizeof(double))
    {
        ReSize();
    }

    *(double*)(_chpBuffer + _writePos) = dValue;
    _writePos += sizeof(dValue);
    return *this;
}

inline NetSerializeBuffer& NetSerializeBuffer::operator>>(unsigned char& byValue)
{

    if (GetDataSize() < sizeof(unsigned char))
    {
        throw NetMsgException(L"operator >> unsigned char");
    }

    byValue = *(unsigned char*)(_chpBuffer + _readPos);
    _readPos += sizeof(byValue);
    return *this;
}

NetSerializeBuffer& NetSerializeBuffer::operator>>(char& chValue)
{

    if (GetDataSize() < sizeof(char))
    {
        throw NetMsgException(L"operator >> char");
    }

    chValue = *(char*)(_chpBuffer + _readPos);
    _readPos += sizeof(chValue);
    return *this;
}

NetSerializeBuffer& NetSerializeBuffer::operator>>(short& shValue)
{
    if (GetDataSize() < sizeof(char))
    {
        throw NetMsgException(L"operator >> short");
    }

    shValue = *(short*)(_chpBuffer + _readPos);
    _readPos += sizeof(shValue);
    return *this;
}

inline NetSerializeBuffer& NetSerializeBuffer::operator>>(unsigned short& wValue)
{
    if (GetDataSize() < sizeof(char))
    {
        throw NetMsgException(L"operator >> unsigned short");
    }

    wValue = *(unsigned short*)(_chpBuffer + _readPos);
    _readPos += sizeof(wValue);
    return *this;
}

NetSerializeBuffer& NetSerializeBuffer::operator>>(int& iValue)
{
    if (GetDataSize() < sizeof(char))
    {
        throw NetMsgException(L"operator >> int");
    }

    iValue = *(int*)(_chpBuffer + _readPos);
    _readPos += sizeof(iValue);
    return *this;
}
inline NetSerializeBuffer& NetSerializeBuffer::operator>>(long& lValue)
{
    if (GetDataSize() < sizeof(char))
    {
        throw NetMsgException(L"operator >> long");
    }

    lValue = *(long*)(_chpBuffer + _readPos);
    _readPos += sizeof(lValue);
    return *this;
}

inline NetSerializeBuffer& NetSerializeBuffer::operator>>(unsigned int& dwValue)
{
    if (GetDataSize() < sizeof(char))
    {
        throw NetMsgException(L"operator >> unsigned int");
    }

    dwValue = *(unsigned int*)(_chpBuffer + _readPos);
    _readPos += sizeof(dwValue);
    return *this;
}

NetSerializeBuffer& NetSerializeBuffer::operator>>(float& fValue)
{
    if (GetDataSize() < sizeof(char))
    {
        throw NetMsgException(L"operator >> float");
    }

    fValue = *(float*)(_chpBuffer + _readPos);
    _readPos += sizeof(fValue);
    return *this;
}

NetSerializeBuffer& NetSerializeBuffer::operator>>(__int64& iValue)
{
    if (GetDataSize() < sizeof(char))
    {
        throw NetMsgException(L"operator >> __int64");
    }

    iValue = *(__int64*)(_chpBuffer + _readPos);
    _readPos += sizeof(iValue);
    return *this;
}

inline NetSerializeBuffer& NetSerializeBuffer::operator>>(unsigned __int64& iValue)
{
    if (GetDataSize() < sizeof(char))
    {
        throw NetMsgException(L"operator >> unsigned __int64");
    }

    iValue = *(unsigned __int64*)(_chpBuffer + _readPos);
    _readPos += sizeof(iValue);
    return *this;
}

NetSerializeBuffer& NetSerializeBuffer::operator>>(double& dValue)
{
    if (GetDataSize() < sizeof(char))
    {
        throw NetMsgException(L"operator >> double");
    }

    dValue = *(double*)(_chpBuffer + _readPos);
    _readPos += sizeof(dValue);
    return *this;

}

int NetSerializeBuffer::GetData(char* chpDest, int copy_size)
{

    if (GetDataSize() < copy_size)
    {
        WCHAR err_msg[50];
        wsprintf(err_msg, L"operator >> GetData copy_size:%d", copy_size);
        throw NetMsgException(err_msg);
    }

    memcpy(chpDest, (_chpBuffer + _readPos), copy_size);
    _readPos += copy_size;

    return copy_size;
}

int NetSerializeBuffer::PutData(char* chpSrc, int copy_size)
{
    while (GetFreeSize() < copy_size)
    {
        ReSize();
    }
    memcpy((_chpBuffer + _writePos), chpSrc, copy_size);

    _writePos += copy_size;
    return copy_size;
}
#endif


#define NET_PACKET_FREE(packet) packet->RemoveCnt()

#pragma warning(default: 4244)
