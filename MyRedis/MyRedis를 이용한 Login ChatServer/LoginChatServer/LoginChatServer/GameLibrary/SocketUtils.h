#pragma once
#include "NetAddress.h"
class SocketUtils
{
public:
	//네트워크 Init 함수
	static void WSAInit();
	static bool WSAClear();
	static bool SocketWIndowsFunction(SOCKET socket, GUID guid, LPVOID* fn);


	static SOCKET CreateSocket();
	static bool Bind(SOCKET socket, SOCKADDR_IN& server_addr);
	static bool BindAnyAddr(SOCKET socket, uint16 port);
	static bool Listen(SOCKET socket, int32 backlog = SOMAXCONN);
	static bool Connect(SOCKET socket, SOCKADDR_IN& server_addr);

	static bool SetLinger(SOCKET socket, uint16 onoff, uint16 timeout);
	static bool SetKeepAlive(SOCKET socket, bool onoff);
	static bool SetSendBufferSize(SOCKET socket, int buf_size);
	static bool SetRecvBufferSize(SOCKET socket, int buf_size);
	static bool SetNagle(SOCKET socket, bool onoff);

	__forceinline static bool CopySocketAttribute(SOCKET client_socket, SOCKET listen_socket)
	{
		return ::setsockopt(client_socket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, reinterpret_cast<char*>(&listen_socket), sizeof(SOCKET)) != SOCKET_ERROR;
	}

public:
	static LPFN_DISCONNECTEX	DisconnectEx;
	static LPFN_ACCEPTEX		AcceptEx;

};



