#include <ntifs.h>
#include <ntimage.h>

#define WORD USHORT
#define DWORD ULONG 
#define MAKELONG(a, b) ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))

typedef struct _IDTR {
	USHORT   IDT_limit;
	USHORT   IDT_LOWbase;
	USHORT   IDT_HIGbase;
}IDTR, *PIDTR;

typedef struct _IDTENTRY
{
	unsigned short LowOffset;       //isr低位地址
	unsigned short selector;
	unsigned char unused_lo;
	unsigned char segment_type : 4;   //0x0E is an interrupt gate
	unsigned char system_segment_flag : 1;
	unsigned char DPL : 2;          // descriptor privilege level 
	unsigned char P : 1;             /* present */
	unsigned short HiOffset;       //isr高位地址
} IDTENTRY, *PIDTENTRY;

VOID EnumIDT()
{
	ULONG_PTR uIndex;
	IDTR idtr;
	IDTENTRY *idt_entry;
	_asm 
	{
		SIDT idtr
	}
	KdPrint(("%d", idtr.IDT_limit));
	idt_entry = (IDTENTRY*)MAKELONG(idtr.IDT_LOWbase, idtr.IDT_HIGbase);
	for (uIndex = 0; uIndex <= idtr.IDT_limit/sizeof(IDTENTRY); uIndex++)
	{
		KdPrint(("%d:0x%x", uIndex, MAKELONG(idt_entry[uIndex].LowOffset, idt_entry[uIndex].HiOffset)));
	}
}

BOOLEAN HookInterrupt()
{
	ULONG_PTR			u_fnKeSetTimeIncrement;
	UNICODE_STRING		usFileName;
	ULONG_PTR			u_index;
	ULONG_PTR			*u_KiProcessorBlock;

	IDTENTRY			*pIdtEntry;
	RtlInitUnicodeString(&usFileName, L"KeSetTimeIncrement");
	u_fnKeSetTimeIncrement = (ULONG_PTR)MmGetSystemRoutineAddress(&usFileName);
	if (!MmIsAddressValid((PVOID)u_fnKeSetTimeIncrement))
	{
		return FALSE;
	}
	u_KiProcessorBlock = *(ULONG_PTR**)(u_fnKeSetTimeIncrement + 44);
	u_index = 0;
	while (u_KiProcessorBlock[u_index])
	{
		pIdtEntry = (PIDTENTRY)u_KiProcessorBlock[u_index] - 0xE8;
		KdPrint(("pIdtEntry:%X", pIdtEntry));
		u_index++;
	}
	return TRUE;
}

NTSTATUS DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("驱动卸载成功！"));
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	KdPrint(("驱动加载成功！"));
	//EnumIDT();
	HookInterrupt();
	pDriverObject->DriverUnload = DriverUnLoad;
	return STATUS_SUCCESS;
}
