#include "ReadWrite.h"

NTSTATUS ReadProcessMemory(PVOID VirtualAddress, SIZE_T Length, PVOID pIoBuffer)
{
	PAGED_CODE();
	NTSTATUS		Status = STATUS_SUCCESS;
	PMDL			Mdl;
	PVOID			MapBuffer;
	Mdl = IoAllocateMdl(VirtualAddress, (ULONG)Length, FALSE, FALSE, NULL);
	if (!Mdl)
	{
		KdPrint(("IoAllocateMdl Failed !"));
		Status = STATUS_INSUFFICIENT_RESOURCES;
		return Status;
	}
	try
	{

		//
		// Probe and lock the pages of this buffer in physical memory.
		// You can specify IoReadAccess, IoWriteAccess or IoModifyAccess
		// Always perform this operation in a try except block.
		//  MmProbeAndLockPages will raise an exception if it fails.
		//
		MmProbeAndLockPages(Mdl, KernelMode, IoReadAccess);
	}
	except(EXCEPTION_EXECUTE_HANDLER)
	{
		KdPrint(("MmProbeAndLockPages Failed !"));
		Status = GetExceptionCode();
		IoFreeMdl(Mdl);
		return Status;
	}
	MapBuffer = MmMapLockedPages(Mdl, KernelMode);

	if (!MapBuffer) {
		KdPrint(("MmMapLockedPages Failed !"));
		Status = STATUS_INSUFFICIENT_RESOURCES;
		IoFreeMdl(Mdl);
		return Status;
	}
	//开始读
	RtlCopyMemory(pIoBuffer, MapBuffer, Length);
	MmUnmapLockedPages(MapBuffer, Mdl);
	MmUnlockPages(Mdl);
	IoFreeMdl(Mdl);
	return Status;
}


NTSTATUS WriteProcessMemory(PVOID VirtualAddress, SIZE_T Length, PVOID pIoBuffer)
{
	PAGED_CODE();
	NTSTATUS		Status = STATUS_SUCCESS;
	PMDL			Mdl;
	PVOID			MapBuffer;
	Mdl = IoAllocateMdl(VirtualAddress, (ULONG)Length, FALSE, FALSE, NULL);
	if (!Mdl)
	{
		KdPrint(("IoAllocateMdl Failed !"));
		Status = STATUS_INSUFFICIENT_RESOURCES;
		return Status;
	}
	try
	{

		//
		// Probe and lock the pages of this buffer in physical memory.
		// You can specify IoReadAccess, IoWriteAccess or IoModifyAccess
		// Always perform this operation in a try except block.
		//  MmProbeAndLockPages will raise an exception if it fails.
		//
		MmProbeAndLockPages(Mdl, KernelMode, IoReadAccess);
	}
	except(EXCEPTION_EXECUTE_HANDLER)
	{
		KdPrint(("MmProbeAndLockPages Failed !"));
		Status = GetExceptionCode();
		IoFreeMdl(Mdl);
		return Status;
	}
	MapBuffer = MmMapLockedPages(Mdl, KernelMode);

	if (!MapBuffer) {
		KdPrint(("MmMapLockedPages Failed !"));
		Status = STATUS_INSUFFICIENT_RESOURCES;
		IoFreeMdl(Mdl);
		return Status;
	}
	//开始写
	RtlCopyMemory(MapBuffer, (PVOID)((ULONG_PTR)pIoBuffer+sizeof(WRIO)), Length); 
	MmUnmapLockedPages(MapBuffer, Mdl);
	MmUnlockPages(Mdl);
	IoFreeMdl(Mdl);
	return Status;

}



ULONG_PTR GetMoudleHandle(LPCWSTR ModuleName) {
	ULONG_PTR Base = 0;

	PPEB Peb = PsGetProcessPeb(Process);
	if (!Peb)
		return Base;
	__try
	{
		if (!Peb->Ldr || !Peb->Ldr->Initialized)
			return Base;


		UNICODE_STRING usModuleName;
		RtlInitUnicodeString(&usModuleName, ModuleName);
		for (PLIST_ENTRY List = Peb->Ldr->InLoadOrderModuleList.Flink;
			List != &Peb->Ldr->InLoadOrderModuleList;
			List = List->Flink) {

			PLDR_DATA_TABLE_ENTRY Entry = CONTAINING_RECORD(List, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
			if (RtlCompareUnicodeString(&Entry->BaseDllName, &usModuleName, TRUE) == 0) {
				Base = (ULONG_PTR)Entry->DllBase;
				break;
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		KdPrint(("EXCEPTION_EXECUTE_HANDLER getmoudle !"));
	}
	return Base;
}