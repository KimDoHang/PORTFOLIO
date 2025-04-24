#pragma once
#include <iostream>
#include <Windows.h>

#ifndef  __PACKET__
#define  __PACKET__


class CSerializeBuffer
{
public:

    /*---------------------------------------------------------------
    Packet Enum.

    ----------------------------------------------------------------*/
    enum en_PACKET
    {
        eBUFFER_DEFAULT = 1400		// 패킷의 기본 버퍼 사이즈.
    };

    //////////////////////////////////////////////////////////////////////////
    // 생성자, 파괴자.
    //
    // Return:
    //////////////////////////////////////////////////////////////////////////
    __inline CSerializeBuffer();
    __inline CSerializeBuffer(int iBufferSize);

    __inline virtual	~CSerializeBuffer();


    //////////////////////////////////////////////////////////////////////////
    // 패킷 청소.
    //
    // Parameters: 없음.
    // Return: 없음.
    //////////////////////////////////////////////////////////////////////////
    __inline void	Clear(void);


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
    __inline int		GetDataSize(void) { return _writePos - _readPos; }
    __inline int		GetFreeSize(void) { return _endPos - _writePos; }


    //////////////////////////////////////////////////////////////////////////
    // 버퍼 포인터 얻기.
    //
    // Parameters: 없음.
    // Return: (char *)버퍼 포인터.
    //////////////////////////////////////////////////////////////////////////
    __inline char* GetBufferPtr(void) { return _chpBuffer; }

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
    __forceinline CSerializeBuffer& operator = (CSerializeBuffer& clSrcPacket);

    //////////////////////////////////////////////////////////////////////////
    // 넣기.	각 변수 타입마다 모두 만듬.
    //////////////////////////////////////////////////////////////////////////
    __forceinline CSerializeBuffer& operator << (unsigned char byValue);
    __forceinline CSerializeBuffer& operator << (char chValue);

    __forceinline CSerializeBuffer& operator << (short shValue);
    __forceinline CSerializeBuffer& operator << (unsigned short wValue);

    __forceinline CSerializeBuffer& operator << (int iValue);
    __forceinline CSerializeBuffer& operator << (long lValue);
    __forceinline CSerializeBuffer& operator << (unsigned int dwValue);
    __forceinline CSerializeBuffer& operator << (float fValue);

    __forceinline CSerializeBuffer& operator << (__int64 iValue);
    __forceinline CSerializeBuffer& operator << (double dValue);


    //////////////////////////////////////////////////////////////////////////
    // 빼기.	각 변수 타입마다 모두 만듬.
    //////////////////////////////////////////////////////////////////////////
    __forceinline CSerializeBuffer& operator >> (unsigned char& byValue);
    __forceinline CSerializeBuffer& operator >> (char& chValue);

    __forceinline CSerializeBuffer& operator >> (short& shValue);
    __forceinline CSerializeBuffer& operator >> (unsigned short& wValue);

    __forceinline CSerializeBuffer& operator >> (int& iValue);
    __forceinline CSerializeBuffer& operator >> (long& lValue);
    __forceinline CSerializeBuffer& operator >> (unsigned int& dwValue);
    __forceinline CSerializeBuffer& operator >> (float& fValue);

    __forceinline CSerializeBuffer& operator >> (__int64& iValue);
    __forceinline CSerializeBuffer& operator >> (double& dValue);

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


protected:

    int	_iBUfferSize;
    //------------------------------------------------------------
    // 현재 버퍼에 사용중인 사이즈.
    //------------------------------------------------------------
    char* _chpBuffer;

    char* _readPos;
    char* _writePos;
    char* _endPos;


};
//////////////////////////////////////////////////////////////////////////

CSerializeBuffer::CSerializeBuffer()
{

    _chpBuffer = new char[eBUFFER_DEFAULT];
    _iBUfferSize = eBUFFER_DEFAULT;
    _readPos = _writePos = _chpBuffer;
    _endPos = _chpBuffer + _iBUfferSize;
}

CSerializeBuffer::CSerializeBuffer(int iBufferSize)
{
    _chpBuffer = new char[iBufferSize];
    _iBUfferSize = iBufferSize;
    _readPos = _writePos = _chpBuffer;
    _endPos = _chpBuffer + _iBUfferSize;

}

CSerializeBuffer::~CSerializeBuffer()
{
    delete[] _chpBuffer;
}

void CSerializeBuffer::Clear(void)
{
    _readPos = _writePos = _chpBuffer;

}

//저장하고 버릴것이므로 
int CSerializeBuffer::MoveWritePos(int iSize)
{
    if (GetFreeSize() < iSize)
        return 0;

    _writePos += iSize;

    return iSize;
}

int CSerializeBuffer::MoveReadPos(int iSize)
{
    if (GetDataSize() < iSize)
        return 0;

    _readPos += iSize;

    return iSize;
}

CSerializeBuffer& CSerializeBuffer::operator=(CSerializeBuffer& clSrcPacket)
{
    memcpy_s(_chpBuffer, _iBUfferSize, clSrcPacket._chpBuffer, clSrcPacket._writePos - clSrcPacket._readPos);
    _writePos = clSrcPacket._writePos;
    return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(unsigned char byValue)
{

#ifdef __BITEWISE__
    * _writePos++ = byValue;
#else
    memcpy_s(_writePos, GetBufferSize(), &byValue, sizeof(byValue));
    _writePos += sizeof(byValue);
#endif
    return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(char chValue)
{
#ifdef __BITEWISE__
    * _writePos++ = chValue;
#else
    memcpy_s(_writePos, GetBufferSize(), &chValue, sizeof(chValue));
    _writePos += sizeof(chValue);
#endif
    return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(short shValue)
{
#ifdef __BITEWISE__
    * _writePos++ = shValue;
    *_writePos++ = shValue >> 8;

#else
    memcpy_s(_writePos, GetBufferSize(), &shValue, sizeof(shValue));
    _writePos += sizeof(shValue);

#endif
    return *this;

}

CSerializeBuffer& CSerializeBuffer::operator<<(unsigned short wValue)
{
#ifdef __BITEWISE__
    * _writePos++ = wValue;
    *_writePos++ = wValue >> 8;
#else
    memcpy_s(_writePos, GetBufferSize(), &wValue, sizeof(wValue));
    _writePos += sizeof(wValue);

#endif
    return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(int iValue)
{
#ifdef __BITEWISE__
    * _writePos++ = iValue;
    *_writePos++ = iValue >> 8;
    *_writePos++ = iValue >> 16;
    *_writePos++ = iValue >> 24;
#else
    memcpy_s(_writePos, GetBufferSize(), &iValue, sizeof(iValue));
    _writePos += sizeof(iValue);

#endif
    return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(long lValue)
{
#ifdef __BITEWISE__
    * _writePos++ = lValue;
    *_writePos++ = lValue >> 8;
    *_writePos++ = lValue >> 16;
    *_writePos++ = lValue >> 24;
#else
    memcpy_s(_writePos, GetBufferSize(), &lValue, sizeof(lValue));
    _writePos += sizeof(lValue);

#endif
    return *this;
}

inline CSerializeBuffer& CSerializeBuffer::operator<<(unsigned int dwValue)
{
#ifdef __BITEWISE__
    * _writePos++ = dwValue;
    *_writePos++ = dwValue >> 8;
    *_writePos++ = dwValue >> 16;
    *_writePos++ = dwValue >> 24;
#else
    memcpy_s(_writePos, GetBufferSize(), &dwValue, sizeof(dwValue));
    _writePos += sizeof(dwValue);

#endif
    return *this;

}

CSerializeBuffer& CSerializeBuffer::operator<<(float fValue)
{
#ifdef __BITEWISE__
    * _writePos++ = fValue;
    *_writePos++ = fValue >> 8;
    *_writePos++ = fValue >> 16;
    *_writePos++ = fValue >> 24;
#else
    memcpy_s(_writePos, GetBufferSize(), &fValue, sizeof(fValue));
    _writePos += sizeof(fValue);

#endif
    return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(__int64 iValue)
{
#ifdef __BITEWISE__
    * _writePos++ = iValue;
    *_writePos++ = iValue >> 8;
    *_writePos++ = iValue >> 16;
    *_writePos++ = iValue >> 24;
    *_writePos++ = iValue >> 32;
    *_writePos++ = iValue >> 40;
    *_writePos++ = iValue >> 48;
    *_writePos++ = iValue >> 56;
#else
    memcpy_s(_writePos, GetBufferSize(), &iValue, sizeof(iValue));
    _writePos += sizeof(iValue);

#endif
    return *this;
}

CSerializeBuffer& CSerializeBuffer::operator<<(double dValue)
{
#ifdef __BITEWISE__
    * _writePos++ = dValue;
    *_writePos++ = dValue >> 8;
    *_writePos++ = dValue >> 16;
    *_writePos++ = dValue >> 24;
    *_writePos++ = dValue >> 32;
    *_writePos++ = dValue >> 40;
    *_writePos++ = dValue >> 48;
    *_writePos++ = dValue >> 56;
#else
    memcpy_s(_writePos, GetBufferSize(), &dValue, sizeof(dValue));
    _writePos += sizeof(dValue);

#endif
    return *this;
}

inline CSerializeBuffer& CSerializeBuffer::operator>>(unsigned char& byValue)
{
#ifdef __BITEWISE__
    byValue = *_readPos++;
#else
    memcpy_s(&byValue, sizeof(byValue), _readPos, sizeof(byValue));
    _readPos += sizeof(byValue);

#endif
    return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(char& chValue)
{
#ifdef __BITEWISE__
    chValue = *_readPos++;
#else
    memcpy_s(&chValue, sizeof(chValue), _readPos, sizeof(chValue));
    _readPos += sizeof(chValue);

#endif
    return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(short& shValue)
{
#ifdef __BITEWISE__
    shValue = *_readPos++;
    shValue >> 8 = *_readPos++;
#else
    memcpy_s(&shValue, sizeof(shValue), _readPos, sizeof(shValue));
    _readPos += sizeof(shValue);

#endif
    return *this;
}

inline CSerializeBuffer& CSerializeBuffer::operator>>(unsigned short& wValue)
{
#ifdef __BITEWISE__
    wValue = *_readPos++;
    wValue >> 8 = *_readPos++;
#else
    memcpy_s(&wValue, sizeof(wValue), _readPos, sizeof(wValue));
    _readPos += sizeof(wValue);

#endif
    return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(int& iValue)
{
#ifdef __BITEWISE__
    iValue = *_readPos++;
    iValue = (*_readPos++) << 8;
    iValue = (*_readPos++) << 16;
    iValue = (*_readPos++) << 24;
#else
    memcpy_s(&iValue, sizeof(iValue), _readPos, sizeof(iValue));
    _readPos += sizeof(iValue);

#endif
    return *this;
}

inline CSerializeBuffer& CSerializeBuffer::operator>>(long& lValue)
{
#ifdef __BITEWISE__
    lValue = *_readPos++;
    lValue = (*_readPos++) << 8;
    lValue = (*_readPos++) << 16;
    lValue = (*_readPos++) << 24;
#else
    memcpy_s(&lValue, sizeof(lValue), _readPos, sizeof(lValue));
    _readPos += sizeof(lValue);

#endif
    return *this;
}

inline CSerializeBuffer& CSerializeBuffer::operator>>(unsigned int& dwValue)
{
#ifdef __BITEWISE__
    dwValue = *_readPos++;
    dwValue = (*_readPos++) << 8;
    dwValue = (*_readPos++) << 16;
    dwValue = (*_readPos++) << 24;
#else
    memcpy_s(&dwValue, sizeof(dwValue), _readPos, sizeof(dwValue));
    _readPos += sizeof(dwValue);

#endif
    return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(float& fValue)
{
#ifdef __BITEWISE__
    fValue = *_readPos++;
    fValue >> 8 = *_readPos++;
    fValue >> 16 = *_readPos++;
    fValue >> 24 = *_readPos++;
#else
    memcpy_s(&fValue, sizeof(fValue), _readPos, sizeof(fValue));
    _readPos += sizeof(fValue);

#endif
    return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(__int64& iValue)
{
#ifdef __BITEWISE__
    iValue = *_readPos++;
    iValue >> 8 = *_readPos++;
    iValue >> 16 = *_readPos++;
    iValue >> 24 = *_readPos++;
    iValue >> 32 = *_readPos++;
    iValue >> 40 = *_readPos++;
    iValue >> 48 = *_readPos++;
    iValue >> 56 = *_readPos++;

#else
    memcpy_s(&iValue, sizeof(iValue), _readPos, sizeof(iValue));
    _readPos += sizeof(iValue);

#endif
    return *this;
}

CSerializeBuffer& CSerializeBuffer::operator>>(double& dValue)
{
#ifdef __BITEWISE__
    dValue = *_readPos++;
    dValue >> 8 = *_readPos++;
    dValue >> 16 = *_readPos++;
    dValue >> 24 = *_readPos++;
    dValue >> 32 = *_readPos++;
    dValue >> 40 = *_readPos++;
    dValue >> 48 = *_readPos++;
    dValue >> 56 = *_readPos++;
#else
    memcpy_s(&dValue, sizeof(dValue), _readPos, sizeof(dValue));
    _readPos += sizeof(dValue);

#endif
    return *this;

}

int CSerializeBuffer::GetData(char* chpDest, int iSize)
{
    if (GetDataSize() < iSize)
        return 0;

    memcpy_s(chpDest, iSize, _readPos, iSize);
    _readPos += iSize;

    return iSize;
}

int CSerializeBuffer::PutData(char* chpSrc, int iSrcSize)
{
    if (GetFreeSize() < iSrcSize)
        return 0;
    memcpy_s(_writePos, iSrcSize, chpSrc, iSrcSize);
    _writePos += iSrcSize;

    return iSrcSize;
}
#endif
