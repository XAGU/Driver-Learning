#include <ntifs.h>

//01 96c577ec 83e758b4 nt!ObpGetObjectSecurity+0x8e
//01 9663b3bc 83e72346 nt!ObpAssignSecurity+0x77
//01 96667b7c 83cc5d60 nt!ObpRemoveObjectRoutine+0x44

//global
ULONG_PTR g_orig_hookpointer;
ULONG_PTR g_goto_origpointer;
UCHAR g_corig_pointercode[5];
BOOLEAN g_bhook_success;

ULONG_PTR SeDefaultObjectMethod;

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

BOOLEAN Jmp_HookFunction(
	IN ULONG_PTR Destination,
	IN ULONG_PTR Source,
	IN UCHAR *Ori_code
	)
{
	ULONG_PTR jmp_offset;
	UCHAR jmp_code[5] = { 0xE9 };

	KSPIN_LOCK lock;
	KIRQL irql;

	if (Destination==0||Source==0)
	{
		DbgPrint("Params error!");
		return FALSE;
	}
	RtlCopyMemory(Ori_code, (PVOID)Destination, 5);
	jmp_offset = Source - Destination - 5;
	*(ULONG_PTR*)&jmp_code[1] = jmp_offset;

	KeInitializeSpinLock(&lock);
	KeAcquireSpinLock(&lock, &irql);

	OffPageProtect();
	RtlCopyMemory((PVOID)Destination, jmp_code, 5);
	OnPageProtect();

	KeReleaseSpinLock(&lock, irql);
	return TRUE;
}

VOID Res_HookFunction(
	IN ULONG_PTR Destination,
	IN UCHAR *Ori_code,
	IN ULONG_PTR Length
)
{
	KSPIN_LOCK lock;
	KIRQL irql;

	if (Destination == 0 || Ori_code == 0)
	{
		return;
	}
	KeInitializeSpinLock(&lock);
	KeAcquireSpinLock(&lock, &irql);

	OffPageProtect();
	RtlCopyMemory((PVOID)Destination, Ori_code, Length);
	OnPageProtect();

	KeReleaseSpinLock(&lock, irql);
}

ULONG_PTR FilterGetObjectSecurity(ULONG_PTR ObjectType)
{
	if (ObjectType==(ULONG_PTR)*PsThreadType||ObjectType==(ULONG_PTR)*PsProcessType)
	{
		KdPrint(("FilterGetObjectSecurity"));
		return 1;
	}
	return 0;
}

__declspec(naked)
VOID NewGetObjectSecurity()
{
	__asm
	{
		pushfd
		pushad

		push esi
		call FilterGetObjectSecurity
		test eax,eax
		je __exit

		popad
		popfd

		cmp eax,eax
		jmp g_goto_origpointer

__exit:
		popad
		popfd
		push eax
		mov eax,SeDefaultObjectMethod
		cmp [esi+0x6c],eax
		pop eax
		jmp g_goto_origpointer
	}
}

VOID PassObjectHook()
{
	ULONG_PTR ObpGetObjectSecurity;
	ObpGetObjectSecurity = (ULONG_PTR)ObGetObjectSecurity + 17;
	ObpGetObjectSecurity += *(ULONG_PTR*)(ObpGetObjectSecurity + 1) + 5;
	if (!MmIsAddressValid((PVOID)ObpGetObjectSecurity))
	{
		return;
	}
	g_orig_hookpointer = ObpGetObjectSecurity + 39; 
	g_goto_origpointer = g_orig_hookpointer + 7;

	SeDefaultObjectMethod = *(ULONG_PTR*)(g_orig_hookpointer + 3);

	g_bhook_success = Jmp_HookFunction(
		g_orig_hookpointer,
		(ULONG_PTR)NewGetObjectSecurity,
		g_corig_pointercode);
}

VOID UnPassObjectHook()
{
	if (g_bhook_success)
	{
		Res_HookFunction(g_orig_hookpointer, g_corig_pointercode, 5);
	}
}

NTSTATUS DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	UnPassObjectHook();
	KdPrint(("驱动卸载成功！"));
	return STATUS_SUCCESS;
}



NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	PassObjectHook();
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("驱动加载成功！"));
	return STATUS_SUCCESS;
}