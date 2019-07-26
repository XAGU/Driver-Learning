#include "ReadWrite.h"
#include "ForceDelete.h"

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
		KdPrint(("<--->IoCreateDevice Failed !"));
		return Status;
	}
	MyDevice->Flags |= DO_BUFFERED_IO;
	RtlInitUnicodeString(&usSymName, SYM_LINK_NAME);
	Status = IoCreateSymbolicLink(&usSymName, &usDeviceName);
	if (!NT_SUCCESS(Status))
	{
		IoDeleteDevice(MyDevice);
		KdPrint(("<--->IoCreateSymbolicLink Failed !"));
		return Status;
	}
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
	case IOCTL_INIT:
	{
		KdPrint(("<--->IOCTL_INIT"));
		Status = PsLookupProcessByProcessId(*(HANDLE*)pIoBuffer, &Process);
		if (!NT_SUCCESS(Status))
		{
			KdPrint(("PsLookupProcessByProcessId Failed !"));
			info = 0;
			break;
		}
		ObDereferenceObject(Process);
		info = OutLength;
	}
	break;
	case IOCTL_READ:
	{
		KAPC_STATE		ApcState;
		KdPrint(("<--->IOCTL_READ"));
		if (!Process)
		{
			info = 0;
		}
		else
		{
			KeStackAttachProcess(Process, &ApcState);
			Status = ReadProcessMemory(((PWRIO)pIoBuffer)->VirtualAddress, ((PWRIO)pIoBuffer)->Length, pIoBuffer);
			KeUnstackDetachProcess(&ApcState);
			if (!NT_SUCCESS(Status))
			{
				KdPrint(("<--->IOCTL_READ Failed !"));
				info = 0;
			}
			else
			{
				info = OutLength;
			}
		}
	}
	break;
	case IOCTL_WRITE:
	{
		KAPC_STATE		ApcState;
		KdPrint(("<--->IOCTL_WRITE"));
		if (!Process)
		{
			info = 0;
		}
		else
		{
			KeStackAttachProcess(Process, &ApcState);
			Status = WriteProcessMemory(((PWRIO)pIoBuffer)->VirtualAddress, ((PWRIO)pIoBuffer)->Length, pIoBuffer);
			KeUnstackDetachProcess(&ApcState);
			if (!NT_SUCCESS(Status))
			{
				KdPrint(("<--->IOCTL_WRITE Failed !"));
				info = 0;
			}
			else
			{
				info = OutLength;
			}
		}
	}
	break;
	case IOCTL_GMODHAN:
	{
		if (!Process)
		{
			info = 0;
		}
		else
		{
			ULONG_PTR		Handle = 0;
			KAPC_STATE		ApcState;
			RtlZeroMemory((PVOID)((ULONG_PTR)pIoBuffer + InLength), 2);
			KeStackAttachProcess(Process, &ApcState);
			Handle = GetMoudleHandle((LPCWSTR)pIoBuffer);
			KeUnstackDetachProcess(&ApcState);
			*(ULONG_PTR*)pIoBuffer = Handle;
			info = OutLength;
		}
	}
	break;
	case IOCTL_FORCEDELETE:
	{
		RtlZeroMemory((PVOID)((ULONG_PTR)pIoBuffer + InLength), 2);
		if (!ForceDelete((LPCWSTR)pIoBuffer))
		{
			info = 0;
		}
		else
		{
			info = OutLength;
		}
	}
	break;
	default:
		KdPrint(("<--->CODE ERROR !"));
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
	UNICODE_STRING	usSymName;
	RtlInitUnicodeString(&usSymName, SYM_LINK_NAME);
	if (pDriverObject->DeviceObject != NULL)
	{
		IoDeleteSymbolicLink(&usSymName);
		IoDeleteDevice(pDriverObject->DeviceObject);
	}
	KdPrint(("<--->驱动加载成功！"));
}


DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	NTSTATUS				Status;
	//创建设备对象
	Status = CreateDevice(pDriverObject);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("<--->创建device失败！"));
	}
	else
	{
		KdPrint(("<--->创建device成功！"));
		KdPrint(("<--->%wZ", pRegPath));
	}
	SetDispatchFunction(pDriverObject);
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("<--->驱动加载成功！"));
	return STATUS_SUCCESS;
}