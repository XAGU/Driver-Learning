#include "Common.h"
#include "HidePCHunter.h"


VOID DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("驱动卸载成功！"));
}

DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	NTSTATUS				Status;
	PDRIVER_OBJECT			pPcHunterDriver;
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("驱动加载成功！"));
	Status = GetDriverObjectByName(&pPcHunterDriver, L"\\Driver\\PCHunter64as");
	if (pPcHunterDriver == NULL)
		return Status;
	KdPrint(("pPcHunterDriver : %llX", pPcHunterDriver));
	IoRegisterDriverReinitialization(pPcHunterDriver, HideDriver, NULL);
	//HidePCHDriverDepsSelf(pDriverObject);
	return STATUS_SUCCESS;
}