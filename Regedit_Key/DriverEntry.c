#include <ntddk.h>

NTSTATUS CreateKey()
{
	NTSTATUS Status;
	HANDLE hRegister;
	OBJECT_ATTRIBUTES ObjAttr;
	UNICODE_STRING usKeyName,usValueKey;
	ULONG Disposition;
	DWORD64 value = 9;
	RtlZeroMemory(&ObjAttr, sizeof(OBJECT_ATTRIBUTES));
	RtlInitUnicodeString(&usKeyName, L"\\REGISTRY\\MACHINE\\SYSTEM\\ControlSet001\\services\\MyKey");
	InitializeObjectAttributes(&ObjAttr, &usKeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	Status = ZwCreateKey(&hRegister, KEY_ALL_ACCESS, &ObjAttr, 0, NULL, REG_OPTION_NON_VOLATILE, &Disposition);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("创建注册表失败:%X",Status));
		return Status;
	}
	if (Disposition == REG_CREATED_NEW_KEY)
	{
		KdPrint(("创建注册表成功！"));
	}
	else
	{
		KdPrint(("打开注册表成功！"));
	}

	RtlInitUnicodeString(&usValueKey, L"Name");
	ZwSetValueKey(hRegister, &usValueKey, NULL, REG_DWORD, &value, sizeof(DWORD32));
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("File Code:%X", Status));
		return Status;
	}
	ZwClose(hRegister);
	return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT pDriver)
{
	KdPrint(("驱动卸载成功!"));
}


NTSTATUS OpenKey()
{
	NTSTATUS Status;
	HANDLE hRegister;
	OBJECT_ATTRIBUTES ObjAttr;
	UNICODE_STRING usKeyName;
	RtlZeroMemory(&ObjAttr, sizeof(OBJECT_ATTRIBUTES));
	RtlInitUnicodeString(&usKeyName, L"\\REGISTRY\\MACHINE\\SYSTEM\\ControlSet001\\services\\MyKey");
	InitializeObjectAttributes(&ObjAttr, &usKeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	Status = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &ObjAttr);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("打开注册表失败:%X", Status));
		return Status;
	}
	KdPrint(("打开注册表成功！"));
	ZwClose(hRegister);
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pRegPath)
{
	pDriver->DriverUnload = DriverUnload;
	KdPrint(("驱动加载成功!"));
	KdPrint(("%wZ", pRegPath));
	//创建注册表
	CreateKey();
	//打开注册表
	// OpenKey();
	return STATUS_SUCCESS;
}