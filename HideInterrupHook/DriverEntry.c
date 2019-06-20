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

 

NTSTATUS DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("驱动卸载成功！"));
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	KdPrint(("驱动加载成功！"));
	pDriverObject->DriverUnload = DriverUnLoad;
	return STATUS_SUCCESS;
}