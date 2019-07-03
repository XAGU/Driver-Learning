#include <ntifs.h>

#define PROCESS_ID_OFFSET	0XB4
#define PROCESS_LIST_OFFSET	0XB8

typedef struct _User_Timer
{
	ULONG_PTR	UNKNOWN1;
	ULONG_PTR	UNKNOWN2;
	ULONG_PTR	UNKNOWN3;
	LIST_ENTRY	gList;
	ULONG_PTR	Hwnd_Object;
	ULONG_PTR	ppi;
	ULONG_PTR	TimerId;
	ULONG_PTR	UNKNOWN4;
	ULONG_PTR	uElapse;
	ULONG_PTR	flag;
	ULONG_PTR	lpTimerFunc;
	ULONG_PTR	UNKNOWN5;
	LIST_ENTRY	List_UnKnow1;
	LIST_ENTRY	List_UserTimer;
}User_timer,*pUser_Timer;

typedef VOID (*KEATTACHPROCESS)(PRKPROCESS Process);
typedef VOID (*KEDETACHPROCESS)();


//global
KEATTACHPROCESS KeAttachProcesss;
KEDETACHPROCESS KeDetachProcesss;
PLIST_ENTRY		gtmrListHead;



BOOLEAN InitalizeData()
{
	UNICODE_STRING str_Func1, str_Func2;
	RtlInitUnicodeString(&str_Func1, L"KeAttachProcess");
	RtlInitUnicodeString(&str_Func2, L"KeDetachProcess");
	KeAttachProcesss = (KEATTACHPROCESS)MmGetSystemRoutineAddress(&str_Func1);
	KeDetachProcesss = (KEDETACHPROCESS)MmGetSystemRoutineAddress(&str_Func2);
	if (!MmIsAddressValid(KeAttachProcess)||!MmIsAddressValid(KeDetachProcess))
	{
		return FALSE;
	}
	gtmrListHead = (PLIST_ENTRY)0x94990b50;
	return TRUE;
}

VOID SerchUserTimer()
{
	PLIST_ENTRY pListNext;
	pUser_Timer pTimer;
	PKPROCESS	AttachProcess;
	if (!InitalizeData())
	{
		return;
	}
	AttachProcess = (PKPROCESS)0x882efb20;
	KeAttachProcesss(AttachProcess);
	__try {
		pListNext = gtmrListHead->Flink;
		while (pListNext != gtmrListHead)
		{
			pTimer = CONTAINING_RECORD(pListNext, User_timer, gList);
			KdPrint(("Timer:0x%X\t\t\t\t\t\tTimerID:%8d\t\t\t\t\t\tlpTimerFunc:0x%8X\t\t\t\t\t\tuElapse:%d",
				pTimer, pTimer->TimerId, pTimer->lpTimerFunc, pTimer->uElapse));
			pListNext = pListNext->Flink;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
	//
	}
	KeDetachProcesss(AttachProcess);

}


NTSTATUS DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("驱动卸载成功！"));
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("驱动加载成功！"));
	SerchUserTimer();
	return STATUS_SUCCESS;
}