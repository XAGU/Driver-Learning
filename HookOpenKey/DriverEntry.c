#include <ntifs.h>

#pragma pack(1) //写这个内存以一字节对齐 如果不写是以4字节的对齐的  
typedef struct ServiceDescriptorEntry {//这个结构就是为了管理这个数组而来的 内核api所在的数组 才有这个结构的 这个是ssdt  
	unsigned int *ServiceTableBase;//就是ServiceTable ssdt数组  
	unsigned int *ServiceCounterTableBase; //仅适用于checked build版本 无用  
	unsigned int NumberOfServices;//(ServiceTableBase)数组中有多少个元素 有多少个项  
	unsigned char *ParamTableBase;//参数表基址 我们层传过来的api的参数 占用多少字节 多大  
} ServiceDescriptorTableEntry_t, *PServiceDescriptorTableEntry_t;
#pragma pack()
__declspec(dllimport) ServiceDescriptorTableEntry_t KeServiceDescriptorTable;



//关闭页只读保护
void _declspec(naked) ShutPageProtect()
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
void _declspec(naked) OpenPageProtect()
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

//globle
ULONG_PTR g_Origin_NtOpenKey;
ULONG_PTR g_jmp_NtOpenKey;
UCHAR g_orign_funcode[5];

VOID FilterNtOpenKey()
{
	KdPrint(("%s", (char*)PsGetCurrentProcess() + 0x16c));
}

__declspec(naked)
VOID NewNtOpenKey()
{
	__asm
	{
		call FilterNtOpenKey;
		pop eax;
		mov edi, edi;
		push ebp;
		mov ebp, esp;
		jmp g_jmp_NtOpenKey;
	}

}

VOID HookNtOpenKey()
{
	ULONG_PTR u_jmp_temp;
	UCHAR jmp_code[5];
	g_Origin_NtOpenKey = KeServiceDescriptorTable.ServiceTableBase[182];
	g_jmp_NtOpenKey = g_Origin_NtOpenKey + 5;

	u_jmp_temp = (ULONG_PTR)NewNtOpenKey - g_Origin_NtOpenKey - 5;
	jmp_code[0] = 0xE8;
	*(ULONG_PTR*)&jmp_code[1] = u_jmp_temp;
	ShutPageProtect();
	RtlCopyMemory(g_orign_funcode, (PVOID)g_Origin_NtOpenKey, 5);
	RtlCopyMemory((PVOID)g_Origin_NtOpenKey, jmp_code, 5);
	OpenPageProtect();
}

VOID UnHookNtOpenKey()
{
	ShutPageProtect();
	RtlCopyMemory((PVOID)g_Origin_NtOpenKey, g_orign_funcode, 5);
	OpenPageProtect();
}

VOID DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	UnHookNtOpenKey();
	KdPrint(("驱动卸载成功！"));
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	KdPrint(("驱动加载成功！"));
	HookNtOpenKey();
	pDriverObject->DriverUnload = DriverUnLoad;
	return STATUS_SUCCESS;
}