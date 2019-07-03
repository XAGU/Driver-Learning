#include <ntifs.h>

typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemBasicInformation,
	SystemProcessorInformation,
	SystemPerformanceInformation,
	SystemTimeOfDayInformation,
	SystemPathInformation,
	SystemProcessInformation,
	SystemCallCountInformation,
	SystemDeviceInformation,
	SystemProcessorPerformanceInformation,
	SystemFlagsInformation,
	SystemCallTimeInformation,
	SystemModuleInformation,
	SystemLocksInformation,
	SystemStackTraceInformation,
	SystemPagedPoolInformation,
	SystemNonPagedPoolInformation,
	SystemHandleInformation,
	SystemObjectInformation,
	SystemPageFileInformation,
	SystemVdmInstemulInformation,
	SystemVdmBopInformation,
	SystemFileCacheInformation,
	SystemPoolTagInformation,
	SystemInterruptInformation,
	SystemDpcBehaviorInformation,
	SystemFullMemoryInformation,
	SystemLoadGdiDriverInformation,
	SystemUnloadGdiDriverInformation,
	SystemTimeAdjustmentInformation,
	SystemSummaryMemoryInformation,
	SystemNextEventIdInformation,
	SystemEventIdsInformation,
	SystemCrashDumpInformation,
	SystemExceptionInformation,
	SystemCrashDumpStateInformation,
	SystemKernelDebuggerInformation,
	SystemContextSwitchInformation,
	SystemRegistryQuotaInformation,
	SystemExtendServiceTableInformation,
	SystemPrioritySeperation,
	SystemPlugPlayBusInformation,
	SystemDockInformation,
//	SystemPowerInformation,
	SystemProcessorSpeedInformation,
	SystemCurrentTimeZoneInformation,
	SystemLookasideInformation
} SYSTEM_INFORMATION_CLASS, *PSYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_THREAD {
	LARGE_INTEGER           KernelTime;
	LARGE_INTEGER           UserTime;
	LARGE_INTEGER           CreateTime;
	ULONG                   WaitTime;
	PVOID                   StartAddress;
	CLIENT_ID               ClientId;
	KPRIORITY               Priority;
	LONG                    BasePriority;
	ULONG                   ContextSwitchCount;
	ULONG                   State;
	KWAIT_REASON            WaitReason;

} SYSTEM_THREAD, *PSYSTEM_THREAD;

typedef struct _SYSTEM_PROCESS_INFORMATION {
	ULONG                   NextEntryOffset;
	ULONG                   NumberOfThreads;
	LARGE_INTEGER           Reserved[3];
	LARGE_INTEGER           CreateTime;
	LARGE_INTEGER           UserTime;
	LARGE_INTEGER           KernelTime;
	UNICODE_STRING          ImageName;
	KPRIORITY               BasePriority;
	HANDLE                  ProcessId;
	HANDLE                  InheritedFromProcessId;
	ULONG                   HandleCount;
	ULONG                   Reserved2[2];
	ULONG                   PrivatePageCount;
	VM_COUNTERS             VirtualMemoryCounters;
	IO_COUNTERS             IoCounters;
	SYSTEM_THREAD           Threads[0];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef NTSTATUS (*ZWQUERYSYSTEMINFORMATINON)(
	_In_      SYSTEM_INFORMATION_CLASS SystemInformationClass,
	_Inout_   PVOID                    SystemInformation,
	_In_      ULONG                    SystemInformationLength,
	_Out_opt_ PULONG                   ReturnLength
);


VOID EnumProcess()
{
	NTSTATUS status;
	UNICODE_STRING us_func_name;
	PVOID p_process_imfomation;
	ULONG_PTR retLength;
	ULONG_PTR struct_begin;
	ULONG_PTR uIndex;
	ZWQUERYSYSTEMINFORMATINON ZwQuerySystemInformation;
	PSYSTEM_PROCESS_INFORMATION process_info;
	RtlInitUnicodeString(&us_func_name, L"ZwQuerySystemInformation");
	ZwQuerySystemInformation = (ZWQUERYSYSTEMINFORMATINON)MmGetSystemRoutineAddress(&us_func_name);
	if (ZwQuerySystemInformation==0)
	{
		return;
	}
	
	status = ZwQuerySystemInformation(SystemProcessInformation, 0, 0, &retLength);
	if (status!=STATUS_INFO_LENGTH_MISMATCH)
	{
		return;
	}
	KdPrint(("retLength:%d", retLength));
	p_process_imfomation = ExAllocatePool(NonPagedPool, retLength);
	if (p_process_imfomation==0)
	{
		KdPrint(("p_process_imfomation分配失败！"));
		return;
	}
	status = ZwQuerySystemInformation(SystemProcessInformation, p_process_imfomation, retLength, &retLength);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("ZwQuerySystemInformation失败！\t%X", status));
		ExFreePool(p_process_imfomation);
		return;
	}
	process_info = (SYSTEM_PROCESS_INFORMATION*)p_process_imfomation;
	while (process_info->NextEntryOffset)
	{
		KdPrint(("ImageName:%20wZ\t\t\t\t\t\t\t\t\tProcessId:%8d\t\t\t\t\t\t\t\tHandleCount:%8d", 
			&process_info->ImageName,process_info->ProcessId,process_info->HandleCount));

		for (uIndex = 0; uIndex < process_info->NumberOfThreads; uIndex++)
		{
			KdPrint(("        ThreadID:%10X\t\t\t\t\t\t\t\tStartAddress:%X",
				process_info->Threads[uIndex].ClientId.UniqueThread, process_info->Threads[uIndex].StartAddress));
		}
		process_info = (SYSTEM_PROCESS_INFORMATION*)((ULONG_PTR)process_info + process_info->NextEntryOffset);
	}
	ExFreePool(p_process_imfomation);
}


VOID EnumProcessByLinks()
{
	PLIST_ENTRY		plist_head,plist;
	ULONG_PTR		eprocess;
	eprocess = (ULONG_PTR)PsGetCurrentProcess();
	plist_head = (PLIST_ENTRY)(eprocess + 0xb8);
	plist = plist_head;
	while (plist->Flink != plist_head)
	{
		eprocess = (ULONG_PTR)plist - 0xb8;
		KdPrint(("%s", eprocess + 0x16c));
		plist = plist->Flink;
	}
}

VOID EnumProcessByWindows()
{
	NTSTATUS		status;
	PEPROCESS		gui_process;
	ULONG_PTR		win32_process,tag_desk_top,tag_desk_info,tag_desk_wnd,tagwnd;
	ULONG_PTR		tag_thread_info,ethread, eprocess;
	status = PsLookupProcessByProcessId((HANDLE)1312, &gui_process);
	if (!NT_SUCCESS(status))
	{
		return;
	}
	KeAttachProcess(gui_process);
	do
	{
		win32_process = *(ULONG_PTR*)((ULONG_PTR)gui_process + 0x120);
		if (win32_process == 0)
		{
			break;
		}
		tag_desk_top = *(ULONG_PTR*)(win32_process + 0x98);
		if (tag_desk_top == 0)
		{
			break;
		}
		tag_desk_info = *(ULONG_PTR*)(tag_desk_top + 0x4);
		if (tag_desk_info == 0)
		{
			break;
		}
		tag_desk_wnd = *(ULONG_PTR*)(tag_desk_info + 0x8);
		if (tag_desk_wnd == 0)
		{
			break;
		}
		tagwnd = *(ULONG_PTR*)(tag_desk_wnd + 0x38);
		while (tagwnd)
		{
			tag_thread_info = *(ULONG_PTR*)(tagwnd + 0x8);

			if (tag_thread_info==0)
			{
				tagwnd = *(ULONG_PTR*)(tagwnd + 0x2c);
				continue;
			}

			ethread = *(ULONG_PTR*)tag_thread_info;
			if (ethread==0)
			{
				tagwnd = *(ULONG_PTR*)(tagwnd + 0x2c);
				continue;
			}
			eprocess = *(ULONG_PTR*)(ethread + 0x150);
			if (eprocess==0)
			{
				tagwnd = *(ULONG_PTR*)(tagwnd + 0x2c);
				continue;
			}
			KdPrint(("%s", eprocess + 0x16c));
			tagwnd = *(ULONG_PTR*)(tagwnd + 0x2c);
		}
	} while (0);

	KeDetachProcess();

	ObDereferenceObject(gui_process);
}

NTSTATUS DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("驱动卸载成功！"));
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	//EnumProcess();
	//EnumProcessByLinks();
	EnumProcessByWindows();
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("驱动加载成功！"));
	return STATUS_SUCCESS;
}