#include <ntifs.h>


typedef NTSTATUS
(*SEDEFAULTOBJECTMETHOD)(
	__in PVOID Object,
	__in SECURITY_OPERATION_CODE OperationCode,
	__in PSECURITY_INFORMATION SecurityInformation,
	__inout PSECURITY_DESCRIPTOR SecurityDescriptor,
	__inout_opt PULONG CapturedLength,
	__deref_inout PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
	__in POOL_TYPE PoolType,
	__in PGENERIC_MAPPING GenericMapping,
	__in ULONG_PTR unknown
);

SEDEFAULTOBJECTMETHOD SeDefaultObjectMethod;

NTSTATUS NewSeDefaultObjectMethod(
	__in PVOID Object,
	__in SECURITY_OPERATION_CODE OperationCode,
	__in PSECURITY_INFORMATION SecurityInformation,
	__inout PSECURITY_DESCRIPTOR SecurityDescriptor,
	__inout_opt PULONG CapturedLength,
	__deref_inout PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
	__in POOL_TYPE PoolType,
	__in PGENERIC_MAPPING GenericMapping,
	__in ULONG_PTR unknown
)
{
	NTSTATUS status;
	if (strstr((char*)PsGetCurrentProcess() + 0x16c,"OllyDBG")!=0)
	{
		__asm int 3;
	}
	KdPrint(("process name: %s", (char*)PsGetCurrentProcess() + 0x16c));
	status = SeDefaultObjectMethod(Object,
		OperationCode,
		SecurityInformation,
		SecurityDescriptor,
		CapturedLength,
		ObjectsSecurityDescriptor,
		PoolType,
		GenericMapping,
		unknown);
	return status;
}

VOID SetObjHook()
{
	//SeDefaultObjectMethod = (SEDEFAULTOBJECTMETHOD)*(ULONG_PTR*)((ULONG_PTR)*PsProcessType + 0x6c);
	SeDefaultObjectMethod = (SEDEFAULTOBJECTMETHOD)*(ULONG_PTR*)((ULONG_PTR)*PsProcessType + 0x6c);
	*(ULONG_PTR*)((ULONG_PTR)*PsProcessType + 0x6c) = (ULONG_PTR)NewSeDefaultObjectMethod;
}

VOID ResObjHook()
{
	*(ULONG_PTR*)((ULONG_PTR)*PsProcessType + 0x6c) = (ULONG_PTR)SeDefaultObjectMethod;
}


NTSTATUS DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	ResObjHook();
	KdPrint(("驱动卸载成功！"));
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	SetObjHook();
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("驱动加载成功！"));
	return STATUS_SUCCESS;
}