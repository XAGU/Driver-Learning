#include <ntifs.h>


//global
KTIMER g_kTimer;
KDPC g_kDpc;

typedef struct _KTIMER_TABLE_ENTRY {
	ULONG_PTR			Lock;
	LIST_ENTRY			Entry;
	LARGE_INTEGER		Time;
}KTIMER_TABLE_ENTRY,*PKTIMER_TABLE_ENTRY;

VOID DeferreRoutine(
	_In_ struct _KDPC *Dpc,
	_In_opt_ PVOID DeferredContext,
	_In_opt_ PVOID SystemArgument1,
	_In_opt_ PVOID SystemArgument2
)
{
	LARGE_INTEGER largeInt;
	KdPrint(("DpcTimer Called !"));
	largeInt.QuadPart = -10000000;
	KeSetTimer(&g_kTimer, largeInt, &g_kDpc);
}

VOID SetDpcTimer()
{
	LARGE_INTEGER largeInt;
	KeInitializeTimer(&g_kTimer);//初始化Timer
	KeInitializeDpc(&g_kDpc, DeferreRoutine,NULL);//初始化DPC
	largeInt.QuadPart = -10000000;
	KeSetTimer(&g_kTimer, largeInt, &g_kDpc);
}

VOID CancelTimer()
{
	KeCancelTimer(&g_kTimer);
}

VOID EnumDpcTimer()
{
	ULONG_PTR Index;
	ULONG_PTR kPrcb;
	PKTIMER pKtimer;
	PLIST_ENTRY pList;
	PKTIMER_TABLE_ENTRY kTimerTable;
	__asm {
		push eax
		mov eax,fs:[0x20]
		mov kPrcb,eax
		pop eax
	}
	kTimerTable = (PKTIMER_TABLE_ENTRY)(kPrcb + 0x19A0);
	for (Index = 0; Index < 512; Index++)
	{
		pList = kTimerTable[Index].Entry.Flink;
		if (!MmIsAddressValid(pList))
		{
			continue;
		}
		__try {
			while (pList != &kTimerTable[Index].Entry)
			{
				pKtimer = (PKTIMER)((ULONG_PTR)pList - 0x18);
				if (MmIsAddressValid(pKtimer)&&
					MmIsAddressValid(pKtimer->Dpc))
				{
					if (pKtimer->Period&0xF0000000)
					{
						break;
					}
					KdPrint(("%X----%d", pKtimer->Dpc->DeferredRoutine, pKtimer->Period));
				}
				else
				{
					break;
				}
				pList = pList->Flink;
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			continue;
		}
	}
}

NTSTATUS DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("驱动卸载成功！"));
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	EnumDpcTimer();
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("驱动加载成功！"));
	return STATUS_SUCCESS;
}