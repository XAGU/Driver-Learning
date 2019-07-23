#include "ReadWrite.h"

NTSTATUS ReadProcessMemory(PVOID VirtualAddress, SIZE_T Length, PVOID pIoBuffer)
{
	NTSTATUS		Status = STATUS_SUCCESS;
	PMDL			Mdl;
	PVOID			MapBuffer;
	Mdl = IoAllocateMdl(VirtualAddress, (ULONG)Length, FALSE, FALSE, NULL);
	if (!Mdl)
	{
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

		Status = GetExceptionCode();
		IoFreeMdl(Mdl);
		return Status;
	}
	MapBuffer = MmMapLockedPages(Mdl, KernelMode);

	if (!MapBuffer) {
		Status = STATUS_INSUFFICIENT_RESOURCES;
		IoFreeMdl(Mdl);
		return Status;
	}
	//开始读
	RtlCopyMemory(pIoBuffer, MapBuffer, Length);
	MmUnmapLockedPages(MapBuffer, Mdl);
	return Status;
}


NTSTATUS WriteProcessMemory(PVOID VirtualAddress, SIZE_T Length, PVOID pIoBuffer)
{
	NTSTATUS		Status = STATUS_SUCCESS;
	PMDL			Mdl;
	PVOID			MapBuffer;
	Mdl = IoAllocateMdl(VirtualAddress, (ULONG)Length, FALSE, FALSE, NULL);
	if (!Mdl)
	{
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

		Status = GetExceptionCode();
		IoFreeMdl(Mdl);
		return Status;
	}
	MapBuffer = MmMapLockedPages(Mdl, KernelMode);

	if (!MapBuffer) {
		Status = STATUS_INSUFFICIENT_RESOURCES;
		IoFreeMdl(Mdl);
		return Status;
	}
	//开始写
	RtlCopyMemory(MapBuffer, (PVOID)((ULONG_PTR)pIoBuffer+sizeof(WRIO)), Length);
	MmUnmapLockedPages(MapBuffer, Mdl);
	return Status;

}