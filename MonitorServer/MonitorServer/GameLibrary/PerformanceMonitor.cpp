#include "pch.h"
#include "PerformanceMonitor.h"
#include "Lock.h"

PDH::PDH(const WCHAR* FILE_NAME, const WCHAR* DIR_NAME)
{
	WCHAR path[MAX_PATH];
	wstring exe_name;
	wstring counter_name;

	//성능 모니터 쿼리를 처리하기 위해서는 현재 실행 파일의 경로를 가져와야만 한다.
	//이를 처리하기 위한 함수들이다.
	if (GetModuleFileNameW(nullptr, path, MAX_PATH) == 0) {
		CRASH(L"File EXE Open Fail");
	}

	std::wstring fullPath(path);
	size_t last_slash = fullPath.find_last_of(L"\\/");
	exe_name = fullPath.substr(last_slash + 1);
	size_t dotPos = exe_name.find_last_of(L".");
	exe_name.erase(dotPos, 4);

	if (PdhOpenQuery(NULL, NULL, &_pdh_process_query) != ERROR_SUCCESS)
	{
		__debugbreak();
	}

	PdhOpenQuery(NULL, NULL, &_pdh_hardware_query);

	//프로세스 종속
	counter_name = L"\\Process(" + exe_name + L")" + L"\\Pool Nonpaged Bytes";
	PdhAddCounter(_pdh_process_query, counter_name.c_str(), NULL, &_pdh_process_counter.process_nonpage_memory_counter);

	counter_name = L"\\Process(" + exe_name + L")" + L"\\Private Bytes";
	PdhAddCounter(_pdh_process_query, counter_name.c_str(), NULL, &_pdh_process_counter.process_private_memory_counter);

	counter_name = L"\\Process(" + exe_name + L")" + L"\\Page Faults/sec";
	PdhAddCounter(_pdh_process_query, counter_name.c_str(), NULL, &_pdh_process_counter.process_page_fault_counter);

	counter_name = L"\\Process(" + exe_name + L")" + L"\\Handle Count";
	PdhAddCounter(_pdh_process_query, counter_name.c_str(), NULL, &_pdh_process_counter.process_handle_counter);

	counter_name = L"\\Process(" + exe_name + L")" + L"\\Thread Count";
	PdhAddCounter(_pdh_process_query, counter_name.c_str(), NULL, &_pdh_process_counter.process_thread_num_counter);

	/*counter_name = L"\\Process(" + exe_name + L")" + L"\\Virtual Bytes";
	PdhAddCounter(_pdh_process_query, counter_name.c_str(), NULL, &_pdh_process_counter.process_virtual_memory_counter);

	counter_name = L"\\Process(" + exe_name + L")" + L"\\Working Set";
	PdhAddCounter(_pdh_process_query, counter_name.c_str(), NULL, &_pdh_process_counter.process_working_memory_counter);*/

	//하드웨어
	PdhAddCounter(_pdh_hardware_query, L"\\Memory\\Available MBytes", NULL, &_pdh_hardware_counter.hardware_available_memory_counter);
	PdhAddCounter(_pdh_hardware_query, L"\\Memory\\Pool Nonpaged Bytes", NULL, &_pdh_hardware_counter.hardware_nonpage_memory_counter);


	PdhAddCounter(_pdh_hardware_query, L"\\Memory\\Page Faults/sec", NULL, &_pdh_hardware_counter.hardware_page_fault_counter);
	PdhAddCounter(_pdh_hardware_query, L"\\Memory\\Cache Faults/sec", NULL, &_pdh_hardware_counter.hardware_cache_miss_counter);

	PdhAddCounter(_pdh_hardware_query, L"\\System\\Context Switches/sec", NULL, &_pdh_hardware_counter.hardware_context_switch_counter);

	PdhAddCounter(_pdh_hardware_query, L"\\Network Adapter(Realtek PCIe GBE Family Controller)\\Bytes Received/sec", NULL, &_pdh_hardware_counter.hardware_adapter_recv1_counter);
	PdhAddCounter(_pdh_hardware_query, L"\\Network Adapter(Realtek PCIe GBE Family Controller _2)\\Bytes Received/sec", NULL, &_pdh_hardware_counter.hardware_adapter_recv2_counter);
	PdhAddCounter(_pdh_hardware_query, L"\\Network Adapter(Realtek PCIe GBE Family Controller)\\Bytes Sent/sec", NULL, &_pdh_hardware_counter.hardware_adapter_send1_counter);
	PdhAddCounter(_pdh_hardware_query, L"\\Network Adapter(Realtek PCIe GBE Family Controller _2)\\Bytes Sent/sec", NULL, &_pdh_hardware_counter.hardware_adapter_send2_counter);


	PdhAddCounter(_pdh_hardware_query, L"\\TCPv4\\Segments Retransmitted/sec", NULL, &_pdh_hardware_counter.hardware_tcp_retransmission_counter);
}

PDH::~PDH()
{
	CloseHandle(_pdh_process_query);
	CloseHandle(_pdh_hardware_query);

	//Process
	CloseHandle(_pdh_process_counter.process_nonpage_memory_counter);
	CloseHandle(_pdh_process_counter.process_private_memory_counter);
	CloseHandle(_pdh_process_counter.process_page_fault_counter);

	CloseHandle(_pdh_process_counter.process_handle_counter);
	CloseHandle(_pdh_process_counter.process_thread_num_counter);

	//CloseHandle(_pdh_process_counter.process_virtual_memory_counter);
	//CloseHandle(_pdh_process_counter.process_working_memory_counter);

	//Hardware
	CloseHandle(_pdh_hardware_counter.hardware_context_switch_counter);
	CloseHandle(_pdh_hardware_counter.hardware_page_fault_counter);
	CloseHandle(_pdh_hardware_counter.hardware_cache_miss_counter);

	CloseHandle(_pdh_hardware_counter.hardware_available_memory_counter);
	CloseHandle(_pdh_hardware_counter.hardware_nonpage_memory_counter);

	CloseHandle(_pdh_hardware_counter.hardware_adapter_recv1_counter);
	CloseHandle(_pdh_hardware_counter.hardware_adapter_recv2_counter);
	CloseHandle(_pdh_hardware_counter.hardware_adapter_send1_counter);
	CloseHandle(_pdh_hardware_counter.hardware_adapter_send2_counter);

	CloseHandle(_pdh_hardware_counter.hardware_tcp_retransmission_counter);

}

void PDH::UpdateProcessPDH()
{
	PdhCollectQueryData(_pdh_process_query);
	_pdh_cpu.UpdateProcessCpuTime();
}

void PDH::UdpateHardwarePDH()
{
	PdhCollectQueryData(_pdh_hardware_query);
	_pdh_cpu.UpdateProcessorCpuTime();
}

void PDH::UpdateAllPDH()
{
	PdhCollectQueryData(_pdh_process_query);
	PdhCollectQueryData(_pdh_hardware_query);
	_pdh_cpu.UpdateCpuTime();
}
