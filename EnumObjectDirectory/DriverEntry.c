#include <ntifs.h>


#define MAX_TABLE                        37
#define MAX_OBJECT_COUNT        0x10000
typedef ULONG DEVICE_MAP;
typedef ULONG EX_PUSH_LOCK;
typedef struct _OBJECT_DIRECTORY_ENTRY {                //目录对象里的表的链表 小表  dt _OBJECT_DIRECTORY_ENTRY 8a408cc8 
	struct _OBJECT_DIRECTORY_ENTRY *ChainLink;
	PVOID        Object;
	ULONG        HashValue;
}OBJECT_DIRECTORY_ENTRY, *POBJECT_DIRECTORY_ENTRY;
typedef struct _OBJECT_DIRECTORY {                            //目录对象 大表  dt _object_directory -b 8a405ed0
	POBJECT_DIRECTORY_ENTRY        HashBuckets[MAX_TABLE];
	EX_PUSH_LOCK        Lock;
	DEVICE_MAP                DeviceMap;
	ULONG                        SessionId;
	PVOID                        NamespaceEntry;
	ULONG                        Flags;
}OBJECT_DIRECTORY, *POBJECT_DIRECTORY;
typedef struct _OBJECT_HEADER_NAME_INFO {	//查询内核对象地址 返回的结构 
	POBJECT_DIRECTORY        Directory;
	UNICODE_STRING                Name;
	ULONG                                ReferenceCount;
}OBJECT_HEADER_NAME_INFO, *POBJECT_HEADER_NAME_INFO;
typedef PVOID(*OBQUERYNAMEINFO)(IN PVOID Object);//通过内核对象地址 查询 名字

//global
OBQUERYNAMEINFO ObQueryNameInfo;//这个函数枚举对象目录


VOID EnumObjectDirectory()
{
	UCHAR						type_index;
	ULONG_PTR					u_front, u_rear;
	ULONG_PTR					uIndex;
	UNICODE_STRING				str_func_name;
	POBJECT_DIRECTORY			ObpRootDirectoryObject = (POBJECT_DIRECTORY)0x8b005e00;
	POBJECT_DIRECTORY_ENTRY*	queue = (POBJECT_DIRECTORY_ENTRY*)ExAllocatePool(NonPagedPool, MAX_OBJECT_COUNT*sizeof(ULONG_PTR));
	POBJECT_HEADER_NAME_INFO	pobject_name_info;

	RtlInitUnicodeString(&str_func_name, L"ObQueryNameInfo");
	ObQueryNameInfo = (OBQUERYNAMEINFO)MmGetSystemRoutineAddress(&str_func_name);
	if (!MmIsAddressValid(ObQueryNameInfo))
	{
		KdPrint(("ObQueryNameInfo is null"));
		return;
	}
	if (!MmIsAddressValid(queue))
	{
		KdPrint(("queue is null"));
		return;
	}
	u_front = u_rear = 0;
	for (uIndex = 0; uIndex < MAX_TABLE; uIndex++)
	{
		if (MmIsAddressValid(ObpRootDirectoryObject->HashBuckets[uIndex]))
		{
			u_rear = (u_rear + 1) % MAX_OBJECT_COUNT;
			queue[u_rear] = (POBJECT_DIRECTORY_ENTRY)ObpRootDirectoryObject->HashBuckets[uIndex];
			//KdPrint(("queue[%d]:%X", uIndex, queue[u_rear]));
		}
	}
	while (u_front!=u_rear)
	{
		u_front = (u_front + 1) % MAX_OBJECT_COUNT;
		pobject_name_info = (POBJECT_HEADER_NAME_INFO)ObQueryNameInfo(queue[u_front]->Object);
		if (MmIsAddressValid(pobject_name_info))
		{
			KdPrint(("%X : %wZ", queue[u_front]->Object, &pobject_name_info->Name));
		}
		type_index = *(UCHAR*)((ULONG_PTR)queue[u_front]->Object - 0xc);
		if (type_index==0x3)
		{
			for (uIndex = 0; uIndex < MAX_TABLE; uIndex++)
			{
				if (MmIsAddressValid(((POBJECT_DIRECTORY)queue[u_front]->Object)->HashBuckets[uIndex]))
				{
					u_rear = (u_rear + 1) % MAX_OBJECT_COUNT;
					queue[u_rear] = ((POBJECT_DIRECTORY)queue[u_front]->Object)->HashBuckets[uIndex];
				}
			}
		}
		if (queue[u_front]->ChainLink!=NULL)
		{
			u_rear = (u_rear + 1) % MAX_OBJECT_COUNT;
			queue[u_rear] = queue[u_front]->ChainLink;
		}
	}
	ExFreePool(queue);
}

NTSTATUS DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("驱动卸载成功！"));
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	EnumObjectDirectory();
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("驱动加载成功！"));
	return STATUS_SUCCESS;
}