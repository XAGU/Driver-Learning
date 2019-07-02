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
};

typedef VOID (*KEATTACHPROCESS)(PRKPROCESS Process);
typedef VOID (*KEDETACHPROCESS)();


//global
KEATTACHPROCESS KeAttachProcess;
KEDETACHPROCESS KeDetachProcess;
ULONG_PTR		gtmrListHead;

VOID InitalizeData()
{
	UNICODE_STRING str_Func1, str_Func2;
	RtlInitUnicodeString(&str_Func1, L"KeAttachProcess");
	RtlInitUnicodeString(&str_Func2, L"KeDetachProcess");
	KeAttachProcess = (KEATTACHPROCESS)MmGetSystemRoutineAddress(&str_Func1);
	KeDetachProcess = (KEDETACHPROCESS)MmGetSystemRoutineAddress(&str_Func2);
	if (!MmIsAddressValid(KeAttachProcess)||!MmIsAddressValid(KeDetachProcess))
	{
		return FALSE;
	}
	gtmrListHead = 0xffffffff

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
	return STATUS_SUCCESS;
}