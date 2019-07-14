#include <ntifs.h>

PETHREAD	pThreadObj;
BOOLEAN		bTerminated = FALSE;

extern POBJECT_TYPE *PsThreadType;

VOID R0_Sleep(int milliSeconds)
{
	LARGE_INTEGER		interval;
	interval.QuadPart = (__int64)milliSeconds * -10000;
	KeDelayExecutionThread(KernelMode, FALSE, &interval);
}

VOID ThreadProc(PVOID pContext)
{
	while (TRUE)
	{
		KdPrint(("call !"));
		if (bTerminated)
		{
			break;
		}
		R0_Sleep(1000);
	}
	PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	bTerminated = TRUE;
	KeWaitForSingleObject(pThreadObj, Executive, KernelMode, FALSE, NULL);
	ObDereferenceObject(pThreadObj);
	KdPrint(("驱动加载成功！"));
}

DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	NTSTATUS				Status;
	HANDLE					hThread;
	OBJECT_ATTRIBUTES		ObjAttr;
	InitializeObjectAttributes(&ObjAttr, NULL, OBJ_KERNEL_HANDLE, 0, NULL);
	Status = PsCreateSystemThread(&hThread, THREAD_ALL_ACCESS, &ObjAttr, NULL, NULL, ThreadProc, NULL);
	if (NT_SUCCESS(Status))
	{
		KdPrint(("线程创建成功！"));
		Status = ObReferenceObjectByHandle(hThread, THREAD_ALL_ACCESS, *PsThreadType, KernelMode, &pThreadObj, NULL);
		ZwClose(hThread);
		if (!NT_SUCCESS(Status))
		{
			bTerminated = TRUE;
		}
	}
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("驱动加载成功！"));
	return STATUS_SUCCESS;
}