#include <ntifs.h>
#include <ntimage.h>

#define WORD USHORT
#define DWORD ULONG 
#define MAKELONG(a, b) ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))


//global
ULONG_PTR		g_InterruptFun3;


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

//关闭页只读保护
void _declspec(naked) OffPageProtect()
{
	__asm
	{
		push eax;
		mov eax, cr0;
		and eax, ~0x10000;
		mov cr0, eax;
		pop eax;
		ret;
	}
}

//开启页只读保护
void _declspec(naked) OnPageProtect()
{
	__asm
	{
		push eax;
		mov eax, cr0;
		or eax, 0x10000;
		mov cr0, eax;
		pop eax;
		ret;
	}
}



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
	KdPrint(("pIdtEntry:%X", idt_entry));
	for (uIndex = 0; uIndex <= idtr.IDT_limit/sizeof(IDTENTRY); uIndex++)
	{
		KdPrint(("%d:0x%x", uIndex, MAKELONG(idt_entry[uIndex].LowOffset, idt_entry[uIndex].HiOffset)));
	}
}

VOID _stdcall FilterInterruptFun3()
{
	KdPrint(("CurrentProcess:%s", (char*)PsGetCurrentProcess() + 0x16c));
}


__declspec(naked)
VOID NewInterruptFun3()
{
	__asm
	{
		pushad
		pushfd

		push fs
		push 0x30
		pop fs
		//mov ax,0x23
		//mov ds,ax
		//mov es,ax

		call FilterInterruptFun3

		pop fs
		popfd
		popad
		jmp g_InterruptFun3
	}
}

ULONG_PTR GetInterrupt(ULONG_PTR InterruptIndex)
{
	IDTR idtr;
	IDTENTRY	*pIdtEntry;
	_asm SIDT	idtr;
	pIdtEntry = (IDTENTRY*)MAKELONG(idtr.IDT_LOWbase, idtr.IDT_HIGbase);
	return MAKELONG(pIdtEntry[InterruptIndex].LowOffset, pIdtEntry[InterruptIndex].HiOffset);
}

VOID SetInterrupt(ULONG_PTR InterrupIndex, ULONG_PTR NewInterruptFunc)
{
	ULONG_PTR			u_fnKeSetTimeIncrement;
	UNICODE_STRING		usFuncName;
	ULONG_PTR			u_index;
	ULONG_PTR			*u_KiProcessorBlock;

	IDTENTRY			*pIdtEntry;
	RtlInitUnicodeString(&usFuncName, L"KeSetTimeIncrement");
	u_fnKeSetTimeIncrement = (ULONG_PTR)MmGetSystemRoutineAddress(&usFuncName);
	if (!MmIsAddressValid((PVOID)u_fnKeSetTimeIncrement))
	{
		return;
	}
	u_KiProcessorBlock = *(ULONG_PTR**)(u_fnKeSetTimeIncrement + 44);
	u_index = 0;
	while (u_KiProcessorBlock[u_index])
	{
		pIdtEntry = *(IDTENTRY**)(u_KiProcessorBlock[u_index] - 0xE8);
		OffPageProtect();
		pIdtEntry[InterrupIndex].LowOffset = (USHORT)((ULONG_PTR)NewInterruptFunc & 0xffff);
		pIdtEntry[InterrupIndex].HiOffset = (USHORT)((ULONG_PTR)NewInterruptFunc >> 16);
		//KdPrint(("pIdtEntry:%X", pIdtEntry));
		OnPageProtect();
		u_index++;
	}
}

NTSTATUS DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("驱动卸载成功！"));
	SetInterrupt(3, g_InterruptFun3);
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	KdPrint(("驱动加载成功！"));
	//EnumIDT();
	g_InterruptFun3 = GetInterrupt(3);
	SetInterrupt(3,(ULONG_PTR)NewInterruptFun3);
	pDriverObject->DriverUnload = DriverUnLoad;
	return STATUS_SUCCESS;
}
