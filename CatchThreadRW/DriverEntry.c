#include "Common.h"


NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS		Status;
	PDEVICE_OBJECT	MyDevice;
	UNICODE_STRING	usDeviceName;
	UNICODE_STRING	usSymName;
	RtlInitUnicodeString(&usDeviceName, DEV_NAME);
	Status = IoCreateDevice(pDriverObject, 0, &usDeviceName, FILE_DEVICE_UNKNOWN, 0, TRUE, &MyDevice);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("IoCreateDevice Failed !"));
		return Status;
	}
	MyDevice->Flags |= DO_BUFFERED_IO;
	RtlInitUnicodeString(&usSymName, SYM_LINK_NAME);
	Status = IoCreateSymbolicLink(&usSymName, &usDeviceName);
	if (!NT_SUCCESS(Status))
	{
		IoDeleteDevice(MyDevice);
		KdPrint(("IoCreateSymbolicLink Failed !"));
		return Status;
	}
	return Status;
}

void CreateThreadNotifyRoutine(HANDLE ProcessId, HANDLE ThreadId, BOOLEAN Create)
{
	NTSTATUS Status;
	PEPROCESS p_Process;
	if (bDieTherad == HAVE_THREAD)
	{
		return;
	}
	if (bDieTherad == EXIT_THREAD)
	{
		KeSetEvent(kEvent, 0, FALSE);
		return;
	}
	if (Create == 0)
	{ //Create 等于0 代表结束 
		PsLookupProcessByProcessId(ProcessId, &p_Process);
		if (strstr(PsGetProcessImageFileName(p_Process), "aaa.exe")!=NULL || strstr(PsGetProcessImageFileName(p_Process), "AAA.exe") != NULL)
		{
			bDieTherad = HAVE_THREAD;
			KdPrint(("当前线程ID:%d,死亡线程ID:%d,所属进程ID:%d,进程名称:%s\n", PsGetCurrentThreadId(), ThreadId, ProcessId, PsGetProcessImageFileName(p_Process)));
			KeWaitForSingleObject(kEvent, Executive, KernelMode, FALSE, NULL);
			while (TRUE)
			{
				if (bDieTherad == EXIT_THREAD)
				{
					KeSetEvent(kEvent, 0, FALSE);
					return;
				}
				if (((PWRIO)pIoBuffer)->IsRead)
				{
					//读
					__try
					{
						KdPrint(("reading"));
						ProbeForRead((PVOID)((PWRIO)pIoBuffer)->Address, ((PWRIO)pIoBuffer)->Length, 1);
						RtlCopyMemory(pIoBuffer, (ULONG_PTR*)((PWRIO)pIoBuffer)->Address, ((PWRIO)pIoBuffer)->Length);
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						KdPrint(("read except\n"));
						((PWRIO)pIoBuffer)->Status = FALSE;
						KeSetEvent(kEvent, 0, TRUE);
						KeWaitForSingleObject(kEvent, Executive, KernelMode, FALSE, NULL);
						continue;
					}
				}
				else
				{
					//写
					__try
					{
						ProbeForWrite((PVOID)((PWRIO)pIoBuffer)->Address, ((PWRIO)pIoBuffer)->Length, 1);
						RtlCopyMemory((ULONG_PTR*)(((PWRIO)pIoBuffer)->Address), pIoBuffer, ((PWRIO)pIoBuffer)->Length);
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						KdPrint(("write except\n"));
						((PWRIO)pIoBuffer)->Status = FALSE;
						KeSetEvent(kEvent, 0, TRUE);
						KeWaitForSingleObject(kEvent, Executive, KernelMode, FALSE, NULL);
						continue;
					}
				}
				((PWRIO)pIoBuffer)->Status = TRUE;
				KeSetEvent(kEvent, 0, TRUE);
				KeWaitForSingleObject(kEvent, Executive, KernelMode, FALSE, NULL);
			}
		}
	}
}

NTSTATUS DispatchDeviceControl(PDEVICE_OBJECT pDriverObj, PIRP pIrp)
{
	NTSTATUS Status;
	Status = STATUS_SUCCESS;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG_PTR InLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG_PTR OutLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG_PTR IoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
	pIoBuffer = pIrp->AssociatedIrp.SystemBuffer;
	ULONG_PTR info;
	switch (IoControlCode)
	{
	case IOCTL_READ:
	{
		KdPrint(("IOCTL_READ"));
		KeSetEvent(kEvent, 0, TRUE);
		KeWaitForSingleObject(kEvent, Executive, KernelMode, FALSE, NULL);
		info = OutLength;
	}
	break;
	case IOCTL_WRITE:
	{
		KdPrint(("IOCTL_WRITE"));
		info = OutLength;
	}
	break;
	case IOCTL_GETPID:
	{
		KdPrint(("IOCTL_GETPID"));
		HANDLE Pid;
		Pid = *(HANDLE*)pIoBuffer;
		KdPrint(("Pid: %d", Pid));
		info = OutLength;
	}
	break;
	default:
		KdPrint(("CODE ERROR !"));
		Status = STATUS_UNSUCCESSFUL;
		break;
	}

	pIrp->IoStatus.Status = Status;
	pIrp->IoStatus.Information = info;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return Status;
}


NTSTATUS DispatchFunction(PDEVICE_OBJECT pDriverObj, PIRP pIrp)
{
	NTSTATUS Status;
	Status = STATUS_SUCCESS;
	pIrp->IoStatus.Status = Status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}


VOID SetDispatchFunction(PDRIVER_OBJECT pDriverObject)
{
	for (size_t i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		pDriverObject->MajorFunction[i] = DispatchFunction;
	}
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
}

VOID DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	bDieTherad = EXIT_THREAD;
	KeSetEvent(kEvent, 0, TRUE);
	KeWaitForSingleObject(kEvent, Executive, KernelMode, FALSE, NULL);
	PsRemoveCreateThreadNotifyRoutine(CreateThreadNotifyRoutine);
	UNICODE_STRING	usSymName;
	RtlInitUnicodeString(&usSymName, SYM_LINK_NAME);
	if (pDriverObject->DeviceObject != NULL)
	{
		IoDeleteSymbolicLink(&usSymName);
		IoDeleteDevice(pDriverObject->DeviceObject);
	}
	ExFreePool(kEvent);
	KdPrint(("驱动加载成功！"));
}


DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	NTSTATUS				Status;
	//创建设备对象
	Status = CreateDevice(pDriverObject);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("创建device失败！"));
	}
	else
	{
		KdPrint(("创建device成功！"));
		KdPrint(("%wZ", pRegPath));
	}
	PsSetCreateThreadNotifyRoutine(CreateThreadNotifyRoutine);
	SetDispatchFunction(pDriverObject);
	//初始化同步事件
	kEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));
	KeInitializeEvent(kEvent, SynchronizationEvent, FALSE);
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("驱动加载成功！"));
	return STATUS_SUCCESS;
}