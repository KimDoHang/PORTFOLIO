#include "pch.h"
#include "SocketUtils.h"
#include "LogUtils.h"

LPFN_DISCONNECTEX	SocketUtils::DisconnectEx = nullptr;
LPFN_ACCEPTEX		SocketUtils::AcceptEx = nullptr;

void SocketUtils::WSAInit()
{
    int32 err_code;
    WSADATA wsa_data;

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        WCHAR log_buff[df_LOG_BUFF_SIZE];
        err_code = WSAGetLastError();
        wsprintf(log_buff, L"WSAStartUp Fail [ErrCode:%d]", err_code);
        g_logutils->Log(NET_LOG_DIR, NET_FILE_NAME, L"WSAStartUp Success");
        __debugbreak();
    }

    g_logutils->Log(NET_LOG_DIR, NET_FILE_NAME, L"WSAStartUp Success");

    SOCKET socket = CreateSocket();

    if (SocketWIndowsFunction(socket, WSAID_ACCEPTEX, reinterpret_cast<LPVOID*>(&AcceptEx)) == false)
    {
        WCHAR log_buff[df_LOG_BUFF_SIZE];
        err_code = WSAGetLastError();
        wsprintf(log_buff, L"AcceptEx Function Setting Fail [ErrCode:%d]", err_code);
        wcout << log_buff << '\n';
        __debugbreak();
    }

    if (SocketWIndowsFunction(socket, WSAID_DISCONNECTEX, reinterpret_cast<LPVOID*>(&DisconnectEx)) == false)
    {
        WCHAR log_buff[df_LOG_BUFF_SIZE];
        err_code = WSAGetLastError();
        wsprintf(log_buff, L"DisconnectEx Function Setting Fail [ErrCode:%d]", err_code);
        wcout << log_buff << '\n';
        __debugbreak();
    }

    closesocket(socket);

    return;
}

bool SocketUtils::WSAClear()
{
    return ::WSACleanup();
}

bool SocketUtils::SocketWIndowsFunction(SOCKET socket, GUID guid, LPVOID* fn)
{
    DWORD bytes = 0;
    return SOCKET_ERROR != ::WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), fn, sizeof(*fn), &bytes, NULL, NULL);
}

SOCKET SocketUtils::CreateSocket()
{
    return ::socket(AF_INET, SOCK_STREAM, 0);
}

bool SocketUtils::Bind(SOCKET socket, SOCKADDR_IN& server_addr)
{
    return SOCKET_ERROR != ::bind(socket, reinterpret_cast<SOCKADDR*>(&server_addr), sizeof(SOCKADDR_IN));
}

bool SocketUtils::BindAnyAddr(SOCKET socket, uint16 port)
{
    SOCKADDR_IN sock_addr;
    sock_addr.sin_port = ::htons(port);
    sock_addr.sin_addr.s_addr = ::htonl(INADDR_ANY);
    sock_addr.sin_family = AF_INET;

    return SOCKET_ERROR != ::bind(socket, reinterpret_cast<SOCKADDR*>(&sock_addr), sizeof(SOCKADDR_IN));
}

bool SocketUtils::Listen(SOCKET socket, int32 backlog)
{
    return SOCKET_ERROR != ::listen(socket, backlog);
}

bool SocketUtils::Connect(SOCKET socket, SOCKADDR_IN& server_addr)
{
    return SOCKET_ERROR != connect(socket, reinterpret_cast<SOCKADDR*>(&server_addr), sizeof(SOCKADDR_IN));
}

bool SocketUtils::SetLinger(SOCKET socket, uint16 onoff, uint16 timeout)
{
    LINGER linger;
    linger.l_onoff = onoff;
    linger.l_linger = timeout;
    return SOCKET_ERROR != ::setsockopt(socket, SOL_SOCKET, SO_LINGER, reinterpret_cast<char*>(&linger), sizeof(LINGER));
}

bool SocketUtils::SetKeepAlive(SOCKET socket, bool onoff)
{
    return SOCKET_ERROR != ::setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<char*>(&onoff), sizeof(onoff));
}

bool SocketUtils::SetSendBufferSize(SOCKET socket, int buf_size)
{
    return SOCKET_ERROR != ::setsockopt(socket, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char*>(&buf_size), sizeof(buf_size));
}

bool SocketUtils::SetRecvBufferSize(SOCKET socket, int buf_size)
{
    return SOCKET_ERROR != ::setsockopt(socket, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char*>(&buf_size), sizeof(buf_size));
}

bool SocketUtils::SetNagle(SOCKET socket, bool onoff)
{
    return SOCKET_ERROR != ::setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&onoff), sizeof(onoff));
}


