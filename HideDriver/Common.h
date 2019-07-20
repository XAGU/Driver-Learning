#pragma once
#include <ntifs.h>
#include <ntimage.h>


#define ULONG32TOULONG64(_x_) (_x_|0xFFFFFFFF00000000)

typedef NTSTATUS(__fastcall *MIPROCESSLOADERENTRY)(PVOID pDriverSection, BOOLEAN bLoad);

typedef struct _LDR_DATA_TABLE_ENTRY {
	// Start from Windows XP
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
	union {
		LIST_ENTRY HashLinks;
		struct {
			PVOID SectionPointer;
			ULONG CheckSum;
		};
	};
	union {
		ULONG TimeDateStamp;
		PVOID LoadedImports;
	};
	PVOID EntryPointActivationContext;        //_ACTIVATION_CONTEXT *
	PVOID PatchInformation;

	// Start from Windows Vista
	LIST_ENTRY ForwarderLinks;
	LIST_ENTRY ServiceTagLinks;
	LIST_ENTRY StaticLinks;
	PVOID ContextInformation;
	PVOID OriginalBase;
	LARGE_INTEGER LoadTime;

} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;


extern POBJECT_TYPE * IoDriverObjectType;

NTSYSAPI NTSTATUS NTAPI ObReferenceObjectByName(
	__in PUNICODE_STRING ObjectName,
	__in ULONG Attributes,
	__in_opt PACCESS_STATE AccessState,
	__in_opt ACCESS_MASK DesiredAccess,
	__in POBJECT_TYPE ObjectType,
	__in KPROCESSOR_MODE AccessMode,
	__inout_opt PVOID ParseContext,
	__out PVOID* Object
);


//global
PVOID g_DriverStart;
ULONG g_DriverSize;
PDRIVER_UNLOAD g_DriverUnload;
PDRIVER_INITIALIZE g_DriverInit;
PDEVICE_OBJECT g_DeviceObject;