/**************************************************************
 *  Filename:    DriverEntry.c
 *
 *  Description: 拦截指定驱动通讯
 *
 *  @author:     XAGU
 **************************************************************/

#include <ntddk.h>

#define DEV_DEVICE L"\\Device\\FirstDevice"
#define LINK_NAME L"\\??\\FirstDevice"


NTSTATUS ObReferenceObjectByName(
	__in PUNICODE_STRING ObjectName,
	__in ULONG Attributes,
	__in_opt PACCESS_STATE AccessState,
	__in_opt ACCESS_MASK DesiredAccess,
	__in POBJECT_TYPE ObjectType,
	__in KPROCESSOR_MODE AccessMode,
	__inout_opt PVOID ParseContext,
	__out PVOID *Object
);

extern POBJECT_TYPE *IoDriverObjectType;

//globle
PDRIVER_OBJECT g_FilterDriverObject;
PDRIVER_DISPATCH gfn_OrigReadCompleteRoutine;

NTSTATUS FilterReadCompleteRoutine(
	_In_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP *Irp
)
{
	KdPrint(("IRP_MJ_QUERY_SECURITY coming!"));
	return gfn_OrigReadCompleteRoutine(DeviceObject, Irp);
}

VOID UnFilterDriverRoutine()
{
	if (MmIsAddressValid(g_FilterDriverObject))
	{
		g_FilterDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = gfn_OrigReadCompleteRoutine;
	}
	
}


NTSTATUS FilterDriverQuery()
{
	NTSTATUS status;
	UNICODE_STRING usObjectName;
	RtlInitUnicodeString(&usObjectName, L"\\Driver\\PCHunter64ak");
	status = ObReferenceObjectByName(&usObjectName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		0,
		*IoDriverObjectType,
		KernelMode,
		NULL,
		(PVOID*)&g_FilterDriverObject
	);
	if (!NT_SUCCESS(status))
	{
		return status;
	}
	KdPrint(("0x%llx", g_FilterDriverObject));
	gfn_OrigReadCompleteRoutine = g_FilterDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];
	g_FilterDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FilterReadCompleteRoutine;

	ObDereferenceObject(g_FilterDriverObject);
	return STATUS_SUCCESS;
}


void MyDriverUnload(PDRIVER_OBJECT pDriverObject)
{
	UNICODE_STRING usSymName;
	RtlInitUnicodeString(&usSymName, LINK_NAME);
	if (pDriverObject->DeviceObject != NULL)
	{
		IoDeleteSymbolicLink(&usSymName);
		IoDeleteDevice(pDriverObject->DeviceObject);
		KdPrint(("驱动卸载成功！"));
	}
	UnFilterDriverRoutine();
}

NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	UNICODE_STRING usDevName;
	UNICODE_STRING usSymName;


	RtlInitUnicodeString(&usDevName, DEV_DEVICE);
	status = IoCreateDevice(pDriverObject, 0, &usDevName, FILE_DEVICE_UNKNOWN, 0, TRUE, &pDevObj);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	pDevObj->Flags |= DO_BUFFERED_IO;
	RtlInitUnicodeString(&usSymName, LINK_NAME);
	status = IoCreateSymbolicLink(&usSymName, &usDevName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
	}
	return STATUS_SUCCESS;
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

NTSTATUS WriteCompleteRoutine(PDEVICE_OBJECT pDriverObj, PIRP pIrp)
{
	NTSTATUS status;
	status = STATUS_SUCCESS;
	KdPrint(("Write"));
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS ReadCompleteRoutine(PDEVICE_OBJECT pDriverObj, PIRP pIrp)
{
	NTSTATUS status;
	status = STATUS_SUCCESS;
	KdPrint(("Read"));
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS status;
	KdPrint(("驱动加载成功！"));
	status = CreateDevice(pDriverObject);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("创建device失败！"));
	}
	else
	{
		KdPrint(("创建device成功！"));
		KdPrint(("%wZ", pRegistryPath));
	}

	pDriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCompleteRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseCompleteRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = WriteCompleteRoutine;
	pDriverObject->MajorFunction[IRP_MJ_READ] = ReadCompleteRoutine;
	pDriverObject->DriverUnload = MyDriverUnload;
	FilterDriverQuery();

	return STATUS_SUCCESS;
}