#include "pch.h"
#include "NetAddress.h"

void NetAddress::SetAddress(SOCKADDR_IN& sock_addr, const WCHAR* ip, uint16 port)
{
    memset(&sock_addr, 0, sizeof(SOCKADDR_IN));

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = ::htons(port);

    if (wcscmp(ip, L"INADDR_ANY") == 0)
    {
        sock_addr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
        InetPtonW(AF_INET, ip, &sock_addr.sin_addr);
    }
}

void NetAddress::GetProcessIP(SOCKADDR_IN& sock_addr, WCHAR* ip)
{
    InetNtopW(AF_INET, &sock_addr.sin_addr, ip, IPBUF_SIZE);
}

uint16 NetAddress::GetProcessPort(SOCKADDR_IN& sock_addr)
{
    return ::ntohs(sock_addr.sin_port);
}


