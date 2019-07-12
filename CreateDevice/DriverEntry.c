#include <ntifs.h>


#define DEV_NAME L"\\Device\\MyDevice"
#define SYM_LINK_NAME L"\\??\\MySymLink"

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

NTSTATUS ReadCompleteRoutine(
	_In_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP *Irp
)
{
	NTSTATUS				Status;
	PIO_STACK_LOCATION		Stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG					ulReadLength = Stack->Parameters.Read.Length;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = ulReadLength;
	memset(Irp->AssociatedIrp.SystemBuffer, 0x90, ulReadLength);
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	Status = STATUS_SUCCESS;
	KdPrint(("Read"));
	return Status;
}

NTSTATUS CreateCompleteRoutine(PDEVICE_OBJECT pDriverObj, PIRP pIrp)
{
	NTSTATUS status;
	status = STATUS_SUCCESS;
	KdPrint(("Create"));
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS CloseCompleteRoutine(PDEVICE_OBJECT pDriverObj, PIRP pIrp)
{
	NTSTATUS status;
	status = STATUS_SUCCESS;
	KdPrint(("Close"));
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING	usSymName;
	RtlInitUnicodeString(&usSymName, SYM_LINK_NAME);
	if (pDriverObject->DeviceObject!=NULL)
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
	pDriverObject->MajorFunction[IRP_MJ_READ] = ReadCompleteRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCompleteRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseCompleteRoutine;
	pDriverObject->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}