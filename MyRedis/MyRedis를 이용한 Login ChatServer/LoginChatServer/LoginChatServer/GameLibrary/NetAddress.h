#pragma once
class NetAddress
{

public:
	static void SetAddress(SOCKADDR_IN& sock_addr, const WCHAR* ip, uint16 port);
	static void GetProcessIP(SOCKADDR_IN& sock_addr, WCHAR* ip);
	static uint16 GetProcessPort(SOCKADDR_IN& sock_addr);

	__forceinline static void GetProcessIPPort(SOCKADDR_IN& sock_addr, WCHAR* ip, uint16& port)
	{
		InetNtopW(AF_INET, &sock_addr.sin_addr, ip, IPBUF_SIZE);
		port = ::ntohs(sock_addr.sin_port);
	}

	__forceinline static void GetProcessIPPort(SOCKADDR_IN& sock_addr, char* ip, uint16& port)
	{
		InetNtopA(AF_INET, &sock_addr.sin_addr, ip, IPBUF_SIZE);
		port = ::ntohs(sock_addr.sin_port);
	}

};

