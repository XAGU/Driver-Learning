#include "HideProcess.h"
/*
1.断链

nt!_EPROCESS
   +0x000 Pcb              : _KPROCESS
   +0x160 ProcessLock      : _EX_PUSH_LOCK
   +0x168 CreateTime       : _LARGE_INTEGER
   +0x170 ExitTime         : _LARGE_INTEGER
   +0x178 RundownProtect   : _EX_RUNDOWN_REF
   +0x180 UniqueProcessId  : Ptr64 Void
   +0x188 ActiveProcessLinks : _LIST_ENTRY

   ActiveProcessLinks记录了正在运行中的进程
2.PspCidTable
nt!PsLookupProcessByProcessId+0x21:
fffff800`0196521d 4533e4          xor     r12d,r12d
fffff800`01965220 488bea          mov     rbp,rdx
fffff800`01965223 66ff8fc4010000  dec     word ptr [rdi+1C4h]
fffff800`0196522a 498bdc          mov     rbx,r12
fffff800`0196522d 488bd1          mov     rdx,rcx
fffff800`01965230 488b0d9149edff  mov     rcx,qword ptr [nt!PspCidTable (fffff800`01839bc8)]
fffff800`01965237 e834480200      call    nt!ExMapHandleToPointer (fffff800`01989a70)
fffff800`0196523c 458d6c2401      lea     r13d,[r12+1]

*/

//断链
BOOLEAN RemoveEprocess(PEPROCESS pEProcess)
{
	PLIST_ENTRY pEntry = (PLIST_ENTRY)((ULONG_PTR)pEProcess + 0x188);
	if (!MmIsAddressValid((PVOID64)pEntry))
	{
		return FALSE;
	}
	KdPrint(("%llX", pEntry));
	pList = pEntry->Flink;
	RemoveEntryList(pEntry);
	return TRUE;
}

BOOLEAN AddEprocess(PEPROCESS pEProcess)
{
	PLIST_ENTRY pEntry = (PLIST_ENTRY)((ULONG_PTR)pEProcess + 0x188);
	if (!MmIsAddressValid((PVOID64)pEntry))
	{
		return FALSE;
	}
	KdPrint(("%llX", pEntry));
	InsertTailList(pList, pEntry);
	return TRUE;
}

NTSTATUS HideProcess(HANDLE ProcessId)
{
	NTSTATUS		Status;
	PEPROCESS		pEProcess;
	Status = PsLookupProcessByProcessId(ProcessId, &pEProcess);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}
	RemoveEprocess(pEProcess);
	return Status;
}

NTSTATUS UnHideProcess(HANDLE ProcessId)
{
	NTSTATUS		Status;
	PEPROCESS		pEProcess;
	Status = PsLookupProcessByProcessId(ProcessId, &pEProcess);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}
	AddEprocess(pEProcess);
	return Status;
}

//PspCidTable
ULONG_PTR GetPspCidTable()
{
	ULONG_PTR			FunAddress;
	UNICODE_STRING		usFunName;
	RtlInitUnicodeString(&usFunName, L"PsLookupProcessByProcessId");
	FunAddress = MmGetSystemRoutineAddress(&usFunName);
	return FunAddress;
}