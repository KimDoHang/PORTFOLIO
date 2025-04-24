#pragma once

//윈도우 헤더
#include <windows.h>
#include <mmsystem.h> // <<--- Here we go
#pragma comment (lib, "winmm.lib")
#pragma comment(lib, "Synchronization.lib")

//입출력 헤더
#include <process.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <strsafe.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")


using namespace std;

//소켓 헤더
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

//Debug관련 헤더
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


