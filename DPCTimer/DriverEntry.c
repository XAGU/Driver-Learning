#include <ntifs.h>


//global
KTIMER g_kTimer;
KDPC g_kDpc;

VOID DeferreRoutine(
	_In_ struct _KDPC *Dpc,
	_In_opt_ PVOID DeferredContext,
	_In_opt_ PVOID SystemArgument1,
	_In_opt_ PVOID SystemArgument2
)
{
	LARGE_INTEGER largeInt;
	KdPrint(("DpcTimer Called !"));
	largeInt.QuadPart = -10000000;
	KeSetTimer(&g_kTimer, largeInt, &g_kDpc);
}

VOID SetDpcTimer()
{
	LARGE_INTEGER largeInt;
	KeInitializeTimer(&g_kTimer);//初始化Timer
	KeInitializeDpc(&g_kDpc, DeferreRoutine,NULL);//初始化DPC
	largeInt.QuadPart = -10000000;
	KeSetTimer(&g_kTimer, largeInt, &g_kDpc);
}

VOID CancelTimer()
{
	KeCancelTimer(&g_kTimer);
}

NTSTATUS DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	CancelTimer();
	KdPrint(("驱动卸载成功！"));
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	SetDpcTimer();
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("驱动加载成功！"));
	return STATUS_SUCCESS;
}