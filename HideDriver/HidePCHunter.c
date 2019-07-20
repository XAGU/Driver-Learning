#include "HidePCHunter.h"


NTSTATUS GetDriverObjectByName(PDRIVER_OBJECT *DriverObject, WCHAR *DriverName)
{
	PDRIVER_OBJECT TempObject = NULL;
	UNICODE_STRING u_DriverName = { 0 };
	NTSTATUS Status = STATUS_SUCCESS;

	RtlInitUnicodeString(&u_DriverName, DriverName);
	Status = ObReferenceObjectByName(&u_DriverName, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, &TempObject);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("获取驱动对象%ws失败!错误码是：%x!\n", Status));
		*DriverObject = NULL;
		return Status;
	}
	*DriverObject = TempObject;
	return Status;
}

BOOLEAN SupportSEH(PDRIVER_OBJECT DriverObject)
{
	//因为驱动从链表上摘除之后就不再支持SEH了
	//驱动的SEH分发是根据从链表上获取驱动地址，判断异常的地址是否在该驱动中
	//因为链表上没了，就会出问题
	//学习（抄袭）到的方法是用别人的驱动对象改他链表上的地址

	PDRIVER_OBJECT BeepDriverObject = NULL;;
	PLDR_DATA_TABLE_ENTRY LdrEntry = NULL;

	GetDriverObjectByName(&BeepDriverObject, L"\\Driver\\beep");
	if (BeepDriverObject == NULL)
		return FALSE;

	//MiProcessLoaderEntry这个函数内部会根据Ldr中的DllBase然后去RtlxRemoveInvertedFunctionTable表中找到对应的项
	//之后再移除他，根据测试来讲..这个表中没有的DllBase就没法接收SEH，具体原理还没懂...
	//所以这里用系统的Driver\\beep用来替死...
	LdrEntry = (PLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection;
	LdrEntry->DllBase = BeepDriverObject->DriverStart;
	ObDereferenceObject(BeepDriverObject);
	return TRUE;
}


PVOID GetProcAddress(WCHAR *FuncName)
{
	UNICODE_STRING u_FuncName = { 0 };
	RtlInitUnicodeString(&u_FuncName, FuncName);
	return MmGetSystemRoutineAddress(&u_FuncName);
}

MIPROCESSLOADERENTRY GetMiProcessLoaderEntry()
{
	/*
	MiProcessLoaderEntry在win10中是没有导出的，但其上层函数MmUnloadSystemImage是导出函数，
	所以我们可以利用MmUnloadSystemImage来获取MiProcessLoaderEntry的地址。
	调用堆栈：
	00 ffffb183`3afa3378 fffff800`3ef9b139 nt!MiProcessLoaderEntry
	01 ffffb183`3afa3380 fffff800`3f0901f0 nt!MiUnloadSystemImage+0x2dd
	02 ffffb183`3afa3500 fffff800`3f090130 nt!MmUnloadSystemImage + 0x20

	注意：win7 win8 这些函数都是没有导出的
	Call偏移 = Call地址 - 当前地址 - 0x5
	*/
	ULONG_PTR MmUnloadSystemImage = (ULONG_PTR)GetProcAddress(L"MmUnloadSystemImage");
	ULONG_PTR MiUnloadSystemImage = MmUnloadSystemImage + 0x1C + ULONG32TOULONG64(*(ULONG32*)(MmUnloadSystemImage + 0x1C)) + 0x4;
	return (MIPROCESSLOADERENTRY)(MiUnloadSystemImage + 0x2d9 + ULONG32TOULONG64(*(ULONG32*)(MiUnloadSystemImage + 0x2d9)) + 0x4);
}



VOID InitInLoadOrderLinks(PLDR_DATA_TABLE_ENTRY LdrEntry)
{
	InitializeListHead(&LdrEntry->InLoadOrderLinks);
	InitializeListHead(&LdrEntry->InMemoryOrderLinks);
}


VOID HideDriver(PDRIVER_OBJECT DriverObject, PVOID Context, ULONG Count)
{
	MIPROCESSLOADERENTRY MiProcessLoaderEntry = NULL;

	MiProcessLoaderEntry = GetMiProcessLoaderEntry();
	if (MiProcessLoaderEntry == NULL)
		return;
	KdPrint(("MiProcessLoaderEntry:%llX", (ULONG_PTR)MiProcessLoaderEntry));
	SupportSEH(DriverObject);
	MiProcessLoaderEntry(DriverObject->DriverSection, 0);
	InitInLoadOrderLinks((PLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection);
	
	//pchunter修改DriverSection蓝屏
	//DriverObject->DriverSection = NULL;
	DriverObject->DriverStart = NULL;
	DriverObject->DriverSize = 0;
	DriverObject->DriverUnload = NULL;
	DriverObject->DriverInit = NULL;
	DriverObject->DeviceObject = NULL;
}

NTSTATUS HidePCHDriverDepsSelf(PDRIVER_OBJECT pDriverObject)
{
	//为了辅助分析，应借助PCHunter的力量，所以我们加载的时候先判断PCHunter的驱动是否加载,
	//如果PCHunter的驱动已经被加载，那么我们对它进行隐藏，以躲避EAC的扫描
	//使用自身的druver_section来定位PCHunter的driver_section
	PAGED_CODE()
	NTSTATUS				Status = STATUS_SUCCESS;
	PLDR_DATA_TABLE_ENTRY	SelfSection = (PLDR_DATA_TABLE_ENTRY)pDriverObject->DriverSection;
	PLDR_DATA_TABLE_ENTRY	TempSection;
	UNICODE_STRING			usPcHunter;
	PLDR_DATA_TABLE_ENTRY	PcHunterSection = NULL;
	RtlInitUnicodeString(&usPcHunter, L"PCHUNTER*");
	TempSection = SelfSection;
	__try {
		do
		{
			if (SelfSection->BaseDllName.Buffer != NULL)
			{
				if (FsRtlIsNameInExpression(&usPcHunter, &SelfSection->BaseDllName, TRUE, NULL))
				{
					PcHunterSection = SelfSection;
					break;
				}
				SelfSection = (PLDR_DATA_TABLE_ENTRY)(SelfSection->InLoadOrderLinks.Blink);
			}
		} while (SelfSection->InLoadOrderLinks.Blink != (PLIST_ENTRY)(TempSection));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return STATUS_UNSUCCESSFUL;
	}

	if (PcHunterSection)
	{
		MIPROCESSLOADERENTRY MiProcessLoaderEntry = NULL;
		MiProcessLoaderEntry = GetMiProcessLoaderEntry();
		if (MiProcessLoaderEntry == NULL)
			return STATUS_UNSUCCESSFUL;
		KdPrint(("%llX", (ULONG_PTR)MiProcessLoaderEntry));
		SupportSEH(pDriverObject);
		MiProcessLoaderEntry(PcHunterSection, 0);
		InitInLoadOrderLinks((PLDR_DATA_TABLE_ENTRY)PcHunterSection);
	}
	return Status;
}