#include "Common.h"

VOID R0_Sleep(int milliSeconds)
{
	LARGE_INTEGER		interval;
	interval.QuadPart = (__int64)milliSeconds * -1;
	KeDelayExecutionThread(KernelMode, FALSE, &interval);
}

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
	if (Create == 0)
	{ //Create 等于0 代表结束 
		PsLookupProcessByProcessId(ProcessId, &p_Process);
		if (strstr(PsGetProcessImageFileName(p_Process), "aaa.exe")!=NULL || strstr(PsGetProcessImageFileName(p_Process), "AAA.exe") != NULL)
		{
			bDieTherad = HAVE_THREAD;
			KdPrint(("当前线程ID:%d,死亡线程ID:%d,所属进程ID:%d,进程名称:%s\n", PsGetCurrentThreadId(), ThreadId, ProcessId, PsGetProcessImageFileName(p_Process)));
			while (TRUE)
			{
				if (bDieTherad == EXIT_THREAD)
				{
					return;
				}
				if (IO.Address == 0)
				{
					KdPrint(("not Read\n"));
					R0_Sleep(1);
					continue;
				}
				if (IO.IsRead)
				{
					//读
					__try
					{
						ProbeForRead((PVOID)IO.Address, IO.Length, 4);
						RtlCopyMemory(&Buffer, (ULONG_PTR*)IO.Address, IO.Length);
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						KdPrint(("read except\n"));
						IO.Status = FALSE;
						continue;
					}
				}
				else
				{
					//写
					__try
					{
						ProbeForWrite((PVOID)IO.Address, IO.Length, 4);
						RtlCopyMemory((ULONG_PTR*)IO.Address, &Buffer, IO.Length);
					}
					__except (EXCEPTION_EXECUTE_HANDLER)
					{
						KdPrint(("write except\n"));
						IO.Status = FALSE;
						continue;
					}
				}
				IO.Status = TRUE;
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
	PVOID pIoBuffer = pIrp->AssociatedIrp.SystemBuffer;
	ULONG_PTR info;
	switch (IoControlCode)
	{
	case IOCTL_READ:
	{
		KdPrint(("IOCTL_READ"));
		RtlCopyMemory(&IO, pIoBuffer, InLength);
		while (IO.Status == FALSE)
		{
			R0_Sleep(1);
		}
		RtlCopyMemory(pIoBuffer, &Buffer, OutLength);
		info = OutLength;
		memset(&IO, 0, sizeof(WRIO));
	}
	break;
	case IOCTL_WRITE:
	{
		KdPrint(("IOCTL_WRITE"));
		IO = *(WRIO*)pIoBuffer;
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
	R0_Sleep(200);
	PsRemoveCreateThreadNotifyRoutine(CreateThreadNotifyRoutine);
	UNICODE_STRING	usSymName;
	RtlInitUnicodeString(&usSymName, SYM_LINK_NAME);
	if (pDriverObject->DeviceObject != NULL)
	{
		IoDeleteSymbolicLink(&usSymName);
		IoDeleteDevice(pDriverObject->DeviceObject);
	}
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
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("驱动加载成功！"));
	return STATUS_SUCCESS;
}