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

typedef struct _KGDTENTRY {   //gdt表的元素的结构里
	USHORT  LimitLow;
	USHORT  BaseLow;
	union {
		struct {
			UCHAR   BaseMid;
			UCHAR   Flags1;     // Declare as bytes to avoid alignment
			UCHAR   Flags2;     // Problems.
			UCHAR   BaseHi;
		} Bytes;
		struct {
			ULONG   BaseMid : 8;
			ULONG   Type : 5;
			ULONG   Dpl : 2;
			ULONG   Pres : 1;

			ULONG   LimitHi : 4;
			ULONG   Sys : 1;
			ULONG   Reserved_0 : 1;
			ULONG   Default_Big : 1;
			ULONG   Granularity : 1;
			ULONG   BaseHi : 8;
		} Bits;
	} HighWord;
} KGDTENTRY, *PKGDTENTRY;

//global
USHORT g_uFilterJmp[3];
ULONG_PTR g_uOrigInterruptFunc;

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


VOID _stdcall FilterInterrupt()
{
	KdPrint(("%s", (char*)PsGetCurrentProcess() + 0x16c));
}

__declspec(naked)
VOID NewInterrupt3OfOrigBase()
{
	__asm
	{
		pushad
		pushfd
		push fs
		push 0x30
		pop fs
		call FilterInterrupt
		pop fs
		popfd
		popad

		jmp g_uOrigInterruptFunc
	}
}

__declspec(naked)
VOID NewINterrupt3()
{
	__asm
	{
		jmp fword ptr[g_uFilterJmp]

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

ULONG_PTR GetNewBase(ULONG_PTR NewInterruptFunc, ULONG_PTR OrigInterruptFunc)
{
	return (NewInterruptFunc - OrigInterruptFunc);
}

VOID SetInterrupt(ULONG_PTR InterrupIndex, ULONG_PTR uNewBase,BOOLEAN bisNew)
{
	ULONG_PTR			uGdtSub = 21;
	ULONG_PTR			u_fnKeSetTimeIncrement;
	UNICODE_STRING		usFuncName;
	ULONG_PTR			u_index;
	ULONG_PTR			*u_KiProcessorBlock;

	IDTENTRY			*pIdtEntry;
	PKGDTENTRY			pGdt;
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
		pGdt = *(PKGDTENTRY*)(u_KiProcessorBlock[u_index] - 0xE4);
		OffPageProtect();
		if (bisNew)
		{
			
			pIdtEntry[InterrupIndex].selector = (SHORT)uGdtSub*0x8;
			RtlCopyMemory(&pGdt[uGdtSub], &pGdt[1],sizeof(KGDTENTRY));
			pGdt[uGdtSub].BaseLow = (USHORT)(uNewBase & 0xffff);
			pGdt[uGdtSub].HighWord.Bytes.BaseMid = (UCHAR)((uNewBase >> 16) & 0xffff);
			pGdt[uGdtSub].HighWord.Bytes.BaseHi = (UCHAR)(uNewBase >> 24);
		}
		else
		{
			pIdtEntry[InterrupIndex].selector = 0x8;
			memset(&pGdt[uGdtSub], 0, sizeof(KGDTENTRY));
		}
		OnPageProtect();
		u_index++;
	}
}

VOID HookInterruptFunc(ULONG_PTR InterrupIndex,ULONG_PTR NewInterruptFunc)
{
	ULONG_PTR uNewBase;
	g_uOrigInterruptFunc = GetInterrupt(InterrupIndex);
	uNewBase = NewInterruptFunc - g_uOrigInterruptFunc;
	*(ULONG_PTR*)g_uFilterJmp = (ULONG_PTR)NewInterrupt3OfOrigBase;
	g_uFilterJmp[2] = 0x8;
	SetInterrupt(InterrupIndex, uNewBase,TRUE);
}

VOID UnHookInterruptFunc(ULONG_PTR InterruptIndex)
{
	SetInterrupt(3, 0, FALSE);
}

NTSTATUS DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("驱动卸载成功！"));
	UnHookInterruptFunc(3);
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	KdPrint(("驱动加载成功！"));
	HookInterruptFunc(3, (ULONG_PTR)NewINterrupt3);
	pDriverObject->DriverUnload = DriverUnLoad;
	return STATUS_SUCCESS;
}