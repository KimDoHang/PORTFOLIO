#pragma once
#include "CpuUsage.h"


struct ProcessPDHInfo
{
	//실제 사용되는 유저 메모리 (DLL 등과 같은 공용 커널 메모리 제외)
	PDH_FMT_COUNTERVALUE process_private_memory;

	//프로세스가 사용하는 논페이지풀 -> TCP는 프로세스에 포함 X
	PDH_FMT_COUNTERVALUE process_nonpage_memory;

	//할당된 가상 메모리
	PDH_FMT_COUNTERVALUE process_virtual_memory;

	//특정 시점에 참조된 페이지수 (실제 Swap Page)
	PDH_FMT_COUNTERVALUE process_working_memory;

	//프로세스 전체 핸들
	PDH_FMT_COUNTERVALUE process_handle;

	//프로세스 전체 스레드 수 (blocked 포함)
	PDH_FMT_COUNTERVALUE process_thread_num;

	//프로세스 page fault 수
	PDH_FMT_COUNTERVALUE process_page_fault;
};

struct HardwarePDHInfo
{
	//하드웨어 페이지, 논페이지 메모리 풀
	PDH_FMT_COUNTERVALUE hardware_available_memory;
	PDH_FMT_COUNTERVALUE hardware_nonpage_memory;

	//Cache, Page Fault -> 미스 성능 계산
	PDH_FMT_COUNTERVALUE hardware_page_fault;
	PDH_FMT_COUNTERVALUE hardware_cache_miss;

	//context switch -> 하드웨어 전체 context switch 횟수, 5000 이상이면 문제
	PDH_FMT_COUNTERVALUE hardware_context_switch;
	//Processor Queue Length -> 대기 스레드 수를 실시간으로 볼 수 있다.

	//Adapter send, recv, retransmission segment num (갯수/sec)
	PDH_FMT_COUNTERVALUE hardware_adapter_send1;
	PDH_FMT_COUNTERVALUE hardware_adapter_send2;
	PDH_FMT_COUNTERVALUE hardware_adapter_recv1;
	PDH_FMT_COUNTERVALUE hardware_adapter_recv2;

	PDH_FMT_COUNTERVALUE hardware_tcp_retransmission;
};

struct ProcessPDHCounter
{
	//실제 사용되는 유저 메모리 (DLL 등과 같은 공용 커널 메모리 제외)
	PDH_HCOUNTER process_private_memory_counter;
	//
	PDH_HCOUNTER process_nonpage_memory_counter;
	//할당된 가상 메모리
	PDH_HCOUNTER process_virtual_memory_counter;
	//특정 시점에 참조된 페이지수 (실제 Swap Page)
	PDH_HCOUNTER process_working_memory_counter;
	PDH_HCOUNTER process_handle_counter;
	PDH_HCOUNTER process_thread_num_counter;

	//process page fault
	PDH_HCOUNTER process_page_fault_counter;

};

struct HardwarePDHCounter
{
	//Memory
	PDH_HCOUNTER hardware_available_memory_counter;
	PDH_HCOUNTER hardware_nonpage_memory_counter;

	//Cache, Page Fault
	PDH_HCOUNTER hardware_page_fault_counter;
	PDH_HCOUNTER hardware_cache_miss_counter;

	//context switch
	PDH_HCOUNTER hardware_context_switch_counter;
	//Processor Queue Length -> 대기 스레드 수를 실시간으로 볼 수 있다.

	//Adapater send, recv, retransmission segment num
	PDH_HCOUNTER hardware_adapter_send1_counter;
	PDH_HCOUNTER hardware_adapter_send2_counter;
	PDH_HCOUNTER hardware_adapter_recv1_counter;
	PDH_HCOUNTER hardware_adapter_recv2_counter;

	PDH_HCOUNTER hardware_tcp_retransmission_counter;
};

class PDH
{
public:
	static const int32 LOG_MAX_PATH = 400;
	static const int32 df_MB_UNIT = 1024 * 1024;
	static const int32 df_KB_UNIT = 1024;

	PDH(const WCHAR* FILE_NAME = L"CHAT", const WCHAR* DIR_NAME = L".\\PERFORMANCE\\");
	~PDH();

	void UpdateProcessPDH();
	void UdpateHardwarePDH();
	void UpdateAllPDH();

	ProcessPDHInfo* GetProcessPDH() { return &_pdh_process_info; }
	HardwarePDHInfo* GetHardwarePDH() { return &_pdh_hardware_info; }

	float ProcessorTotal(void) { return _pdh_cpu.ProcessorTotal(); }
	float ProcessorUser(void) { return _pdh_cpu.ProcessorUser(); }
	float ProcessorKernel(void) { return _pdh_cpu.ProcessorKernel(); }
	float ProcessTotal(void) { return _pdh_cpu.ProcessTotal(); }
	float ProcessUser(void) { return _pdh_cpu.ProcessUser(); }
	float ProcessKernel(void) { return _pdh_cpu.ProcessKernel(); }

	double ProcessUserMem_MB()
	{
		PdhGetFormattedCounterValue(_pdh_process_counter.process_private_memory_counter, PDH_FMT_DOUBLE, NULL, &_pdh_process_info.process_private_memory);
		return _pdh_process_info.process_private_memory.doubleValue / df_MB_UNIT;
	}
	double ProcessNonPageMem_MB()
	{
		PdhGetFormattedCounterValue(_pdh_hardware_counter.hardware_nonpage_memory_counter, PDH_FMT_DOUBLE, NULL, &_pdh_hardware_info.hardware_nonpage_memory);
		return _pdh_process_info.process_nonpage_memory.doubleValue / df_MB_UNIT;
	}
	int32 ProcessPageFault()
	{
		PdhGetFormattedCounterValue(_pdh_process_counter.process_page_fault_counter, PDH_FMT_LONG, NULL, &_pdh_process_info.process_page_fault);
		return _pdh_process_info.process_page_fault.longValue;
	}

	double ProcessHandle()
	{
		PdhGetFormattedCounterValue(_pdh_process_counter.process_handle_counter, PDH_FMT_DOUBLE, NULL, &_pdh_process_info.process_handle);
		return _pdh_process_info.process_handle.doubleValue;
	}

	double ProcessThreadNum()
	{
		PdhGetFormattedCounterValue(_pdh_process_counter.process_thread_num_counter, PDH_FMT_DOUBLE, NULL, &_pdh_process_info.process_thread_num);
		return _pdh_process_info.process_thread_num.doubleValue;
	}

	double ProcessorAvailableMem_MB()
	{
		PdhGetFormattedCounterValue(_pdh_hardware_counter.hardware_available_memory_counter, PDH_FMT_DOUBLE, NULL, &_pdh_hardware_info.hardware_available_memory);
		return _pdh_hardware_info.hardware_available_memory.doubleValue;
	}
	double ProcessorNonPageMem_MB()
	{
		PdhGetFormattedCounterValue(_pdh_hardware_counter.hardware_nonpage_memory_counter, PDH_FMT_DOUBLE, NULL, &_pdh_hardware_info.hardware_nonpage_memory);
		return _pdh_hardware_info.hardware_nonpage_memory.doubleValue / df_MB_UNIT;
	}
	

	int32 ProcessorPageFault()
	{
		PdhGetFormattedCounterValue(_pdh_hardware_counter.hardware_page_fault_counter, PDH_FMT_LONG, NULL, &_pdh_hardware_info.hardware_page_fault);
		return _pdh_hardware_info.hardware_page_fault.longValue;
	}
	int32 ProcessorCacheMiss()
	{
		PdhGetFormattedCounterValue(_pdh_hardware_counter.hardware_cache_miss_counter, PDH_FMT_LONG, NULL, &_pdh_hardware_info.hardware_cache_miss);
		return _pdh_hardware_info.hardware_cache_miss.longValue;
	}
	int32 ProcessorContextSwitch()
	{
		PdhGetFormattedCounterValue(_pdh_hardware_counter.hardware_context_switch_counter, PDH_FMT_LONG, NULL, &_pdh_hardware_info.hardware_context_switch);
		return _pdh_hardware_info.hardware_context_switch.longValue;
	}
	
	double ProcessorAdapterSend_KB()
	{
		PdhGetFormattedCounterValue(_pdh_hardware_counter.hardware_adapter_send1_counter, PDH_FMT_DOUBLE, NULL, &_pdh_hardware_info.hardware_adapter_send1);
		PdhGetFormattedCounterValue(_pdh_hardware_counter.hardware_adapter_send2_counter, PDH_FMT_DOUBLE, NULL, &_pdh_hardware_info.hardware_adapter_send2);
		return (_pdh_hardware_info.hardware_adapter_send1.doubleValue + _pdh_hardware_info.hardware_adapter_send2.doubleValue) / df_KB_UNIT;
	}
	double ProcessorAdapterRecv_KB()
	{
		PdhGetFormattedCounterValue(_pdh_hardware_counter.hardware_adapter_recv1_counter, PDH_FMT_DOUBLE, NULL, &_pdh_hardware_info.hardware_adapter_recv1);
		PdhGetFormattedCounterValue(_pdh_hardware_counter.hardware_adapter_recv2_counter, PDH_FMT_DOUBLE, NULL, &_pdh_hardware_info.hardware_adapter_recv2);
		return (_pdh_hardware_info.hardware_adapter_recv1.doubleValue + _pdh_hardware_info.hardware_adapter_recv2.doubleValue) / df_KB_UNIT;
	}
	
	int32 ProcessorTCPRetransmission()
	{
		PdhGetFormattedCounterValue(_pdh_hardware_counter.hardware_tcp_retransmission_counter, PDH_FMT_LONG, NULL, &_pdh_hardware_info.hardware_tcp_retransmission);
		return _pdh_hardware_info.hardware_tcp_retransmission.longValue;
	}

private:
	PDH_HQUERY _pdh_process_query;
	PDH_HQUERY _pdh_hardware_query;
	ProcessPDHCounter _pdh_process_counter;
	HardwarePDHCounter _pdh_hardware_counter;
	ProcessPDHInfo _pdh_process_info;
	HardwarePDHInfo _pdh_hardware_info;
	CpuUsage _pdh_cpu;

	//Performance Out
};

