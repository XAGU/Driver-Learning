/**************************************************************
 *  Filename:    DriverEntry.c
 *
 *  Description: 遍历本机已加载的驱动，并输出到指定文件
 *
 *  @author:     XAGU
 **************************************************************/



#include <ntddk.h>

#define DEV_DEVICE L"\\Device\\FirstDevice"
#define LINK_NAME L"\\??\\FirstDevice"

typedef struct _LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	USHORT LoadCount;
	USHORT TlsIndex;
	union
	{
		LIST_ENTRY HashLinks;
		struct
		{
			PVOID SectionPointer;
			ULONG CheckSum;
		};
	};
	union
	{
		ULONG TimeDateStamp;
		PVOID LoadedImports;
	};
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

NTSTATUS MyCreateFile(PUNICODE_STRING pusBuffer)
{
	NTSTATUS Status;
	HANDLE hFile;
	OBJECT_ATTRIBUTES FileObjAttr;
	UNICODE_STRING usFileName;
	IO_STATUS_BLOCK IoStatusBlock;
	LARGE_INTEGER lInt;
	PWSTR CrLf = L"\r\n";
	//PUCHAR pBuffer;
	memset(&FileObjAttr, 0, sizeof(OBJECT_ATTRIBUTES));
	RtlInitUnicodeString(&usFileName, L"\\??\\C:\\sys.txt");
	InitializeObjectAttributes(&FileObjAttr, &usFileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	Status = ZwCreateFile(&hFile,
		GENERIC_ALL,
		&FileObjAttr,
		&IoStatusBlock,
		0,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_WRITE,
		FILE_OPEN_IF,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0
	);

	if (!NT_SUCCESS(Status))
	{
		KdPrint(("创建文件失败！"));
		return Status;
	}
	KdPrint(("创建文件成功！"));

	lInt.HighPart = -1;
	lInt.LowPart = FILE_WRITE_TO_END_OF_FILE;
	Status = ZwWriteFile(hFile, NULL, NULL, NULL, &IoStatusBlock, pusBuffer->Buffer, pusBuffer->Length, &lInt, NULL);
	Status = ZwWriteFile(hFile, NULL, NULL, NULL, &IoStatusBlock, CrLf, 2 * sizeof(WCHAR), &lInt, NULL);
	if (!NT_SUCCESS(Status))
	{
		KdPrint(("写入文件失败！%x", Status));
		ZwClose(hFile);
		return Status;
	}
	KdPrint(("写入文件成功！%x", Status));
	ZwClose(hFile);
	return Status;
}


void MyDriverUnload(PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("驱动卸载成功！"));
}

VOID EnumLoadOrderDriver(PDRIVER_OBJECT pDriverObject)
{
	LDR_DATA_TABLE_ENTRY *pDataTableEntry;
	PLIST_ENTRY	pList;
	pDataTableEntry = (LDR_DATA_TABLE_ENTRY*)pDriverObject->DriverSection;
	if (!pDataTableEntry)
	{
		return;
	}
	pList = &pDataTableEntry->InLoadOrderLinks;
	do
	{
		UNICODE_STRING usBuffer;
		KdPrint(("%wZ", ((LDR_DATA_TABLE_ENTRY*)pList)->BaseDllName));
		MyCreateFile(&((LDR_DATA_TABLE_ENTRY*)pList)->BaseDllName);
		pList = pList->Blink;
	} while (pList->Blink != &pDataTableEntry->InLoadOrderLinks);
}


NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
	KdPrint(("驱动加载成功！"));
	KdPrint(("EnumLoadOrderDriver"));
	EnumLoadOrderDriver(pDriverObject);
	pDriverObject->DriverUnload = MyDriverUnload;
	return STATUS_SUCCESS;
}