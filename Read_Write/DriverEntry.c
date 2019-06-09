#include <ntifs.h>


NTSTATUS ReadWriteMem()
{
	NTSTATUS Status;
	HANDLE hProcess;
	OBJECT_ATTRIBUTES ObjAttr;
	CLIENT_ID ClientId;
	PVOID AllocateAddress;
	SIZE_T RegionSize = 0xff;
	RtlZeroMemory(&ObjAttr, sizeof(OBJECT_ATTRIBUTES));
	ClientId.UniqueProcess = 2764;
	ClientId.UniqueThread = 0;
	Status = ZwOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &ObjAttr, &ClientId);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("进程打开失败! Code:%X",Status));
		return Status;
	}
	KdPrint(("进程打开成功! Process:%llX", hProcess));

	ZwAllocateVirtualMemory(hProcess, &AllocateAddress, 0, &RegionSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("分配内存失败! Code:%X", Status));
		return Status;
	}
	KdPrint(("分配内存成功! Address:%ld，Address:%lx,Address:%X,Size:%x", AllocateAddress, AllocateAddress, AllocateAddress,RegionSize));

	ZwClose(hProcess);
	return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT pDriver)
{
	KdPrint(("驱动卸载成功!"));
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pRegPath)
{
	ReadWriteMem();
	pDriver->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}