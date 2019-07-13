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


NTSTATUS KillProcess(HANDLE Pid)
{
	NTSTATUS Status = STATUS_SUCCESS;
	HANDLE ProcessHandle;
	CLIENT_ID Client_Id;
	OBJECT_ATTRIBUTES ObjAttr;
	InitializeObjectAttributes(&ObjAttr, 0, 0, 0, 0);
	Client_Id.UniqueProcess = Pid;
	Client_Id.UniqueThread = 0;
	Status = ZwOpenProcess(&ProcessHandle, PROCESS_ALL_ACCESS, &ObjAttr, &Client_Id);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("ZwOpenProcess Failed !"));
		return Status;
	}
	ZwTerminateProcess(ProcessHandle, Status);
	ZwClose(ProcessHandle);
	return Status;
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
		memset(pIoBuffer, 0xAA, OutLength);
		info = OutLength;
	}
	break;
	case IOCTL_WRITE:
	{
		KdPrint(("IOCTL_WRITE"));
		for (size_t i = 0; i < InLength; i++)
		{
			KdPrint(("%X", ((PUCHAR)(pIoBuffer))[i]));
		}
		info = OutLength;
	}
	break;
	case IOCTL_GETPID:
	{
		KdPrint(("IOCTL_GETPID"));
		HANDLE Pid;
		Pid = *(HANDLE*)pIoBuffer;
		KdPrint(("Pid: %d", Pid));
		Status = KillProcess(Pid);
		if (NT_SUCCESS(Status))
		{
			KdPrint(("关闭进程成功！"));
		}
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


VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING	usSymName;
	RtlInitUnicodeString(&usSymName, SYM_LINK_NAME);
	if (pDriverObject->DeviceObject != NULL)
	{
		IoDeleteSymbolicLink(&usSymName);
		IoDeleteDevice(pDriverObject->DeviceObject);
	}
	KdPrint(("驱动卸载成功！"));
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS Status;
	KdPrint(("驱动加载成功！"));
	//创建设备对象
	Status = CreateDevice(pDriverObject);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("创建device失败！"));
	}
	else
	{
		KdPrint(("创建device成功！"));
		KdPrint(("%wZ", pRegistryPath));
	}
	//设置派遣函数
	SetDispatchFunction(pDriverObject);

	//驱动卸载例程
	pDriverObject->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}