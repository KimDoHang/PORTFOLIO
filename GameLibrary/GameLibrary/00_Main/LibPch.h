#pragma once

//������ ���
#include <windows.h>
#include <mmsystem.h> // <<--- Here we go
#pragma comment (lib, "winmm.lib")
#pragma comment(lib, "Synchronization.lib")

//����� ���
#include <process.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <strsafe.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")


using namespace std;

//���� ���
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

//Debug���� ���
#include <crtdbg.h>
#pragma comment(lib, "DbgHelp.Lib")
#include <dbghelp.h>
#include <Psapi.h>

#include <pdh.h>
#pragma comment(lib,"pdh.lib")

#include "Types.h"
#include "Values.h"
#include "LibErrCode.h"
#include "AtomicUtils.h"
#include "LibGlobal.h"
#include "Container.h"
#include "Macro.h"
#include "Profiler.h"


