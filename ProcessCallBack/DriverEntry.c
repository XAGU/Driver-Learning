#include <ntifs.h>

typedef enum _OB_OPEN_REASON {
	ObCreateHandle,
	ObOpenHandle,
	ObDuplicateHandle,
	ObInheritHandle,
	ObMaxOpenReason
} OB_OPEN_REASON;

typedef NTSTATUS(*OB_OPEN_METHOD)(
	IN ULONG Unknown,
	IN OB_OPEN_REASON OpenReason,
	IN PEPROCESS Process OPTIONAL,
	IN PVOID Object,
	IN ACCESS_MASK GrantedAccess,
	IN ULONG HandleCount
	);

//global
ULONG_PTR	g_OrigOpenProcedure;
BOOLEAN		g_bhook_success;

NTSTATUS OpenProcessCallBack(
	IN ULONG Unknown,
	IN OB_OPEN_REASON OpenReason,
	IN PEPROCESS Process OPTIONAL,
	IN PVOID Object,
	IN ACCESS_MASK GrantedAccess,
	IN ULONG HandleCount)
{
	if (strstr((char*)Object + 0x16c,"Dbgview")!=0)
	{
		KdPrint(("source:%s", (char*)PsGetCurrentProcess() + 0x16c));
		KdPrint(("target:%s", (char*)Object + 0x16c));
		return STATUS_UNSUCCESSFUL;
	}
	return ((OB_OPEN_METHOD)g_OrigOpenProcedure)(Unknown,
		OpenReason,
		Process,
		Object,
		GrantedAccess,
		HandleCount);
}

VOID SetProcessCallBack()
{
	g_OrigOpenProcedure = *(ULONG_PTR*)((ULONG_PTR)*PsProcessType + 0x5c);
	*(ULONG_PTR*)((ULONG_PTR)*PsProcessType + 0x5c) = (ULONG_PTR)OpenProcessCallBack;
}

VOID ResProcessCallBack()
{
	*(ULONG_PTR*)((ULONG_PTR)*PsProcessType + 0x5c) = g_OrigOpenProcedure;
}


NTSTATUS DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	ResProcessCallBack();
	KdPrint(("驱动卸载成功！"));
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	SetProcessCallBack();
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("驱动加载成功！"));
	return STATUS_SUCCESS;
}