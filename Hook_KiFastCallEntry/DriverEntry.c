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

typedef NTSTATUS(*FN_NTCREATFILE)(
	_Out_ PHANDLE FileHandle,
	_In_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_ATTRIBUTES ObjectAttributes,
	_Out_ PIO_STATUS_BLOCK IoStatusBlock,
	_In_opt_ PLARGE_INTEGER AllocationSize,
	_In_ ULONG FileAttributes,
	_In_ ULONG ShareAccess,
	_In_ ULONG CreateDisposition,
	_In_ ULONG CreateOptions,
	_In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
	_In_ ULONG EaLength
	);

//globla
ULONG_PTR g_OriginNtCreatFile;
ULONG_PTR g_fasthook_point;
ULONG_PTR g_goto_origfunc;
UCHAR KEYCODE[5] = { 0x2b, 0xe1, 0xc1, 0xe9, 0x02 };


//关闭页只读保护
void _declspec(naked) OnPageProtect()
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
void _declspec(naked) OffPageProtect()
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

ULONG_PTR SerachHookPointer(ULONG_PTR StartAddress)
{
	UCHAR *p = (UCHAR*)StartAddress;
	for (size_t index = 0; index < 200; index++)
	{
		if (*p == KEYCODE[0] &&
			*(p + 1) == KEYCODE[1] &&
			*(p + 2) == KEYCODE[2] &&
			*(p + 3) == KEYCODE[3] &&
			*(p + 4) == KEYCODE[4])
		{
			return (ULONG_PTR)p;
		}
		p--;
	}
	return 0;
}

VOID FilterKiFastCallEntry(ULONG_PTR ServiceTableBase, ULONG_PTR FunIndex)
{
	if (ServiceTableBase == (unsigned int)KeServiceDescriptorTable.ServiceTableBase)
	{
		if (FunIndex == 190)
		{
			KdPrint(("%s", (char*)PsGetCurrentProcess() + 0x16c));
		}
	}
}

_declspec(naked)
VOID NewKiFastCallEntry()
{
	__asm
	{
		pushad
		pushfd

		push eax
		push edi
		call FilterKiFastCallEntry

		popfd
		popad
		sub esp,ecx
		shr ecx,2
		jmp g_goto_origfunc
	}
}

VOID UnHookKiFastCallEntry()
{
	if (g_fasthook_point == 0)
	{
		return;
	}
	OffPageProtect();
	RtlCopyMemory((PVOID)g_fasthook_point, KEYCODE, 5);
	OnPageProtect();
}


VOID HookKiFastCallEntry(ULONG_PTR HookPoint)
{
	ULONG_PTR u_temp;
	UCHAR u_jmp_code[5];
	u_jmp_code[0] = 0xE9;
	u_temp = (ULONG_PTR)NewKiFastCallEntry - HookPoint - 5;
	*(ULONG_PTR*)u_jmp_code[1] = u_temp;
	OffPageProtect();
	RtlCopyMemory((PVOID)HookPoint, u_jmp_code, 5);
	OnPageProtect();
}


NTSTATUS
NTAPI
NewNtCreateFile(
	_Out_ PHANDLE FileHandle,
	_In_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_ATTRIBUTES ObjectAttributes,
	_Out_ PIO_STATUS_BLOCK IoStatusBlock,
	_In_opt_ PLARGE_INTEGER AllocationSize,
	_In_ ULONG FileAttributes,
	_In_ ULONG ShareAccess,
	_In_ ULONG CreateDisposition,
	_In_ ULONG CreateOptions,
	_In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
	_In_ ULONG EaLength
)
{
	ULONG_PTR u_call_retaddr;
	//栈回溯定位
	__asm
	{
		pushad
		mov eax,[ebp + 4]
		mov u_call_retaddr,eax
		popad
	}

	g_fasthook_point = SerachHookPointer(u_call_retaddr);
	if (g_fasthook_point==0)
	{
		KdPrint(("hook点寻找失败!"));
	}

	g_goto_origfunc = g_fasthook_point + 5;
	HookKiFastCallEntry(g_fasthook_point);
	OffPageProtect();
	KeServiceDescriptorTable.ServiceTableBase[66] = (unsigned int)g_OriginNtCreatFile;
	OnPageProtect();

	return ((FN_NTCREATFILE)g_OriginNtCreatFile)(FileHandle,
		DesiredAccess,
		ObjectAttributes,
		IoStatusBlock,
		AllocationSize,
		FileAttributes,
		ShareAccess,
		CreateDisposition,
		CreateOptions,
		EaBuffer,
		EaLength);
}

//HOOK SSDT
VOID SearchKiFastCallEntry()
{
	g_OriginNtCreatFile = KeServiceDescriptorTable.ServiceTableBase[66];
	OffPageProtect();
	KeServiceDescriptorTable.ServiceTableBase[66] = (unsigned int)NewNtCreateFile;
	OnPageProtect();
}

VOID DriverUnLoad(PDRIVER_OBJECT pDiver_Object)
{
	UnHookKiFastCallEntry();
	KdPrint(("驱动卸载成功！"));
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDiver_Object, PUNICODE_STRING pRegPath)
{
	KdPrint(("驱动加载成功！"));
	SearchKiFastCallEntry();
	pDiver_Object->DriverUnload = DriverUnLoad;
	return STATUS_SUCCESS;
}