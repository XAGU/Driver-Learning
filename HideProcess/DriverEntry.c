#include "HideProcess.h"

VOID DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	//UnHideProcess((HANDLE)1192);
	KdPrint(("驱动卸载成功！"));
}

DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	NTSTATUS				Status;
	KdPrint(("驱动加载成功！"));
	//HideProcess((HANDLE)1192);
	ULONG_PTR i = 0xfffff8a000005000;
	while (*(ULONG_PTR*)i != 0xFFFFFA8032451060)
	{
		i += 0x10;
	}
	KdPrint(("%llX", i));
	pDriverObject->DriverUnload = DriverUnLoad;
	return STATUS_SUCCESS;
}