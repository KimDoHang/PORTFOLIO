#include "pch.h"
#include "ChatServer.h"
#include <conio.h>
#include <Windows.h>
#include "Profiler.h"
#include "CrashDump.h"
#include "ChatValues.h"
#include "ThreadManager.h"
CCrashDump dump;


const WCHAR* dir_name = L"PROFILE";
const WCHAR* file_name = L"ChatServerTest";

int main()
{

	g_chatserver = new ChatServer;
	g_chatserver->ChatInit(L"ChatServer.cnf");



	bool bControlMode = false;
	bool bShutdown = false;


	g_chatserver->ChatStart();

	while (true)
	{
		if (_kbhit())
		{
			WCHAR controlKey = _getwch();

			//배운점 확정적으로 처리가 가능한 부분은 뒤에서 처리한다.

			if (L'u' == controlKey || L'U' == controlKey)
			{
				bControlMode = true;

				std::wcout << L"Control Mode : Press D - Profiler Out" << '\n';
				std::wcout << L"Control Mode : Press R - Profiler Reset" << '\n';
				std::wcout << L"Control Mode : Press Q - Quit" << '\n';
				std::wcout << L"Control Mode : Press L - Key Lock" << '\n';

			}

			if ((L'l' == controlKey || L'L' == controlKey) && bControlMode)
			{
				std::wcout << L"Control Lock... : Press U - Control Unlock" << '\n';
				bControlMode = false;
			}

			if ((L'q' == controlKey || L'Q' == controlKey) && bControlMode)
			{
				g_chatserver->Stop();
				bShutdown = true;
			}

			if ((L'd' == controlKey || L'D' == controlKey) && bControlMode)
			{
				std::wcout << L"File Text Out" << '\n';
				PRO_OUT(dir_name, file_name);
			}

			if ((L'r' == controlKey || L'R' == controlKey) && bControlMode)
			{
				std::wcout << L"Profiler Reset" << '\n';
				PRO_RESET();
			}


		}
	}
}



