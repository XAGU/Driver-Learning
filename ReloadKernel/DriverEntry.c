#include <ntifs.h>
#include "ntimage.h"
#include "DriverEntry.h"

#define __Max(a,b) a>b?a:b

#pragma pack(1) //写这个内存以一字节对齐 如果不写是以4字节的对齐的  
typedef struct ServiceDescriptorEntry {//这个结构就是为了管理这个数组而来的 内核api所在的数组 才有这个结构的 这个是ssdt  
	unsigned int *ServiceTableBase;//就是ServiceTable ssdt数组  
	unsigned int *ServiceCounterTableBase; //仅适用于checked build版本 无用  
	unsigned int NumberOfServices;//(ServiceTableBase)数组中有多少个元素 有多少个项  
	unsigned char *ParamTableBase;//参数表基址 我们层传过来的api的参数 占用多少字节 多大  
} ServiceDescriptorTableEntry_t, *PServiceDescriptorTableEntry_t;
#pragma pack()
__declspec(dllimport) ServiceDescriptorTableEntry_t KeServiceDescriptorTable;

typedef NTSTATUS(*FN_NTCREATFILE)(
	_Out_ PHANDLE FileHandle,
	_In_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_ATTRIBUTES ObjectAttributes,
	_Out_ PIO_STATUS_BLOCK IoStatusBlock,
	_In_opt_ PLARGE_INTEGER AllocationSize,
	_In_ ULONG FileAttributes,
	_In_ ULONG ShareAccess,
	_In_ ULONG CreateDisposition,
	_In_ ULONG CreateOptions,
	_In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
	_In_ ULONG EaLength
	);

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

//global
PVOID g_lpVirtualPointer;
ULONG_PTR g_OriginNtCreatFile;
ULONG_PTR g_fasthook_point;
ULONG_PTR g_goto_origfunc;
UCHAR KEYCODE[5] = { 0x2b, 0xe1, 0xc1, 0xe9, 0x02 };
ULONG_PTR g_NewKernelInc;
ServiceDescriptorTableEntry_t *g_pNewServiceTable;

//关闭页只读保护
void _declspec(naked) OffPageProtect()
{
	__asm
	{
		push eax;
		mov eax, cr0;
		and eax, ~0x10000;
		mov cr0, eax;
		pop eax;
		ret;
	}
}

//开启页只读保护
void _declspec(naked) OnPageProtect()
{
	__asm
	{
		push eax;
		mov eax, cr0;
		or eax, 0x10000;
		mov cr0, eax;
		pop eax;
		ret;
	}
}

ULONG_PTR SerachHookPointer(ULONG_PTR StartAddress)
{
	UCHAR *p = (UCHAR*)StartAddress;
	for (size_t index = 0; index < 200; index++)
	{
		if (*p == KEYCODE[0] &&
			*(p + 1) == KEYCODE[1] &&
			*(p + 2) == KEYCODE[2] &&
			*(p + 3) == KEYCODE[3] &&
			*(p + 4) == KEYCODE[4])
		{
			return (ULONG_PTR)p;
		}
		p--;
	}
	return 0;
}

ULONG_PTR FilterKiFastCallEntry(ULONG_PTR ServiceTableBase, ULONG_PTR FunIndex,ULONG OrigFunAddress)
{
	if (ServiceTableBase == (ULONG_PTR)KeServiceDescriptorTable.ServiceTableBase)
	{
		//KdPrint(("%s", (char*)PsGetCurrentProcess() + 0x16c));
		if (strstr((char*)PsGetCurrentProcess() + 0x16c, "cheatengine-i3")!=0)
		{
			return g_pNewServiceTable->ServiceTableBase[FunIndex];
		}
	}
	return OrigFunAddress;
}

_declspec(naked)
VOID NewKiFastCallEntry()
{
	__asm
	{
		pushad
		pushfd

		push edx
		push eax
		push edi
		call FilterKiFastCallEntry
		mov [esp+0x18],eax

		popfd
		popad
		sub esp, ecx
		shr ecx, 2
		jmp g_goto_origfunc
	}
}

VOID UnHookKiFastCallEntry()
{
	if (g_fasthook_point == 0)
	{
		return;
	}
	OffPageProtect();
	RtlCopyMemory((PVOID)g_fasthook_point, KEYCODE, 5);
	OnPageProtect();
}


VOID HookKiFastCallEntry(ULONG_PTR HookPoint)
{
	ULONG_PTR u_temp;
	UCHAR u_jmp_code[5];
	u_jmp_code[0] = 0xE9;
	u_temp = (ULONG_PTR)NewKiFastCallEntry - HookPoint - 5;
	*(ULONG_PTR*)&u_jmp_code[1] = u_temp;
	OffPageProtect();
	RtlCopyMemory((PVOID)HookPoint, u_jmp_code, 5);
	OnPageProtect();
}


NTSTATUS
NTAPI
NewNtCreateFile(
	_Out_ PHANDLE FileHandle,
	_In_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_ATTRIBUTES ObjectAttributes,
	_Out_ PIO_STATUS_BLOCK IoStatusBlock,
	_In_opt_ PLARGE_INTEGER AllocationSize,
	_In_ ULONG FileAttributes,
	_In_ ULONG ShareAccess,
	_In_ ULONG CreateDisposition,
	_In_ ULONG CreateOptions,
	_In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
	_In_ ULONG EaLength
)
{
	ULONG_PTR u_call_retaddr;
	//栈回溯定位
	__asm
	{
		pushad
		mov eax, [ebp + 0x4]
		mov u_call_retaddr, eax
		popad
	}

	g_fasthook_point = SerachHookPointer(u_call_retaddr);
	if (g_fasthook_point == 0)
	{
		KdPrint(("hook点寻找失败!"));
	}
	else
	{
		KdPrint(("hook点寻找成功!"));
	}

	g_goto_origfunc = g_fasthook_point + 5;
	HookKiFastCallEntry(g_fasthook_point);
	OffPageProtect();
	KeServiceDescriptorTable.ServiceTableBase[66] = (unsigned int)g_OriginNtCreatFile;
	OnPageProtect();

	return ((FN_NTCREATFILE)g_OriginNtCreatFile)(FileHandle,
		DesiredAccess,
		ObjectAttributes,
		IoStatusBlock,
		AllocationSize,
		FileAttributes,
		ShareAccess,
		CreateDisposition,
		CreateOptions,
		EaBuffer,
		EaLength);
}

//HOOK SSDT
VOID SearchKiFastCallEntry()
{
	HANDLE				hFile;
	NTSTATUS			status;
	OBJECT_ATTRIBUTES	Obj_Attr;
	UNICODE_STRING		usFileName;
	IO_STATUS_BLOCK		IoStatusBlock;
	RtlInitUnicodeString(&usFileName, L"\\??\\C:\\Windows\\System32\\ntkrnlpa.exe");
	InitializeObjectAttributes(&Obj_Attr, &usFileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
	g_OriginNtCreatFile = KeServiceDescriptorTable.ServiceTableBase[66];
	OffPageProtect();
	KeServiceDescriptorTable.ServiceTableBase[66] = (unsigned int)NewNtCreateFile;
	OnPageProtect();
	status = ZwCreateFile(&hFile,
		FILE_ALL_ACCESS,
		&Obj_Attr,
		&IoStatusBlock,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN,
		FILE_NON_DIRECTORY_FILE,
		NULL,
		0);
	if (NT_SUCCESS(status))
	{
		ZwClose(hFile);
	}
}

VOID SetNewSSDT(PVOID pNewImage,PVOID pOrigImage,ServiceDescriptorTableEntry_t **pNewServiceTable)
{
	ULONG_PTR uIndex;
	ULONG_PTR uNewKernelInc,uOffset;
	ServiceDescriptorTableEntry_t *pNewSSDT;
	uNewKernelInc = (ULONG_PTR)pNewImage - (ULONG_PTR)pOrigImage;
	pNewSSDT = (ServiceDescriptorTableEntry_t*)((ULONG_PTR)&KeServiceDescriptorTable + uNewKernelInc);
	if (!MmIsAddressValid(pNewSSDT))
	{
		KdPrint(("ERROR in pNewSSDT"));
		return;
	}
	pNewSSDT->NumberOfServices = KeServiceDescriptorTable.NumberOfServices;
	uOffset = (ULONG_PTR)KeServiceDescriptorTable.ServiceTableBase - (ULONG_PTR)pOrigImage;
	pNewSSDT->ServiceTableBase = (unsigned int*)((ULONG_PTR)pNewImage + uOffset);
	if (!MmIsAddressValid(pNewSSDT->ServiceTableBase))
	{
		KdPrint(("ERROR in pNewSSDT->ServiceTableBase"));
		return;
	}

	for (uIndex = 0; uIndex < pNewSSDT->NumberOfServices; uIndex++)
	{
		pNewSSDT->ServiceTableBase[uIndex] += uNewKernelInc;
	}
	
	uOffset = (ULONG_PTR)KeServiceDescriptorTable.ParamTableBase - (ULONG_PTR)pOrigImage;
	pNewSSDT->ParamTableBase = (unsigned char*)((ULONG_PTR)pNewImage + uOffset);
	if (!MmIsAddressValid(pNewSSDT->ParamTableBase))
	{
		KdPrint(("ERROR in pNewSSDT->ParamTableBase"));
		return;
	}
	RtlCopyMemory(pNewSSDT->ParamTableBase, KeServiceDescriptorTable.ParamTableBase, pNewSSDT->NumberOfServices * sizeof(char));
	*pNewServiceTable = pNewSSDT;
	KdPrint(("set new ssdt success"));
}

//基址重定位
VOID ReLocMoudle(PVOID pNewImage, PVOID pOrigImage)
{
	ULONG_PTR				uIndex;
	ULONG_PTR				uRelocTableSize;
	USHORT					TypeValue;
	USHORT					*pwOffsetArrayAddress;
	ULONG_PTR				uTypeOffsetArraySize;
	ULONG_PTR				uRelocOffset;
	ULONG_PTR				uRelocAddress;
	IMAGE_DATA_DIRECTORY	ImageDataDirectory;
	PIMAGE_DOS_HEADER		pImageDosHeader;
	PIMAGE_NT_HEADERS		pImageNtHeaders;
	IMAGE_BASE_RELOCATION	*pImageBaseRelocation;
	pImageDosHeader = (PIMAGE_DOS_HEADER)pNewImage;
	pImageNtHeaders = (PIMAGE_NT_HEADERS)((ULONG_PTR)pNewImage + pImageDosHeader->e_lfanew);
	
	uRelocOffset = (ULONG_PTR)pOrigImage - pImageNtHeaders->OptionalHeader.ImageBase;

	ImageDataDirectory = pImageNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
	pImageBaseRelocation = (PIMAGE_BASE_RELOCATION)(ImageDataDirectory.VirtualAddress + (ULONG_PTR)pNewImage);
	uRelocTableSize = ImageDataDirectory.Size;
	while (uRelocTableSize)
	{
		uTypeOffsetArraySize = (pImageBaseRelocation->SizeOfBlock - 
			sizeof(ULONG_PTR)*2) /sizeof(USHORT);
		pwOffsetArrayAddress = pImageBaseRelocation->TypeOffset;
		for (uIndex = 0; uIndex <uTypeOffsetArraySize; uIndex++)
		{
			TypeValue = pwOffsetArrayAddress[uIndex];
			if (TypeValue>>12==IMAGE_REL_BASED_HIGHLOW)
			{
				uRelocAddress = (TypeValue & 0xfff) + pImageBaseRelocation->VirtualAddress + (ULONG_PTR)pNewImage;
				if (!MmIsAddressValid((PVOID)uRelocAddress))
				{
					continue;
				}
				*(PULONG_PTR)uRelocAddress += uRelocOffset;
			}
		}
		uRelocTableSize -= pImageBaseRelocation->SizeOfBlock;
		pImageBaseRelocation = (IMAGE_BASE_RELOCATION*)((ULONG_PTR)pImageBaseRelocation + pImageBaseRelocation->SizeOfBlock);
	}
}

NTSTATUS ReadFileToMemory(wchar_t *strFileName,PVOID *lpVirtualAddress,PVOID pOrigImage)
{
	NTSTATUS				status;
	HANDLE					hFile;
	LARGE_INTEGER			FileOffset;
	UNICODE_STRING			usFileName;
	OBJECT_ATTRIBUTES		ObjAttr;
	IO_STATUS_BLOCK			ioStatusBlock;
	IMAGE_DOS_HEADER		ImageDosHeader;
	IMAGE_NT_HEADERS		ImageNtHeaders;
	IMAGE_SECTION_HEADER	*pImageSectionHeader;
	ULONG_PTR				uIndex;
	PVOID					lpVirtualPointer;
	ULONG_PTR				SecVirtualAddress, SizeOfSection;
	ULONG_PTR				PointerToRawData;
	if (!MmIsAddressValid(strFileName))
	{
		return STATUS_UNSUCCESSFUL;
	}
	RtlInitUnicodeString(&usFileName, strFileName);
	InitializeObjectAttributes(&ObjAttr,
		&usFileName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	status = ZwCreateFile(&hFile,
		FILE_ALL_ACCESS,
		&ObjAttr,
		&ioStatusBlock,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		FILE_SHARE_READ,
		FILE_OPEN,
		FILE_NON_DIRECTORY_FILE,
		NULL,
		0);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("ZwCreateFile Fail:%X", status));
		return status;
	}
	FileOffset.QuadPart = 0;
	status = ZwReadFile(hFile,
		NULL,
		NULL,
		NULL,
		&ioStatusBlock,
		&ImageDosHeader,
		sizeof(IMAGE_DOS_HEADER),
		&FileOffset,
		NULL);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Read ImageDosHeader Fail:%X", status));
		ZwClose(hFile);
		return status;
	}
	FileOffset.QuadPart = ImageDosHeader.e_lfanew;
	status = ZwReadFile(hFile,
		NULL,
		NULL,
		NULL,
		&ioStatusBlock,
		&ImageNtHeaders,
		sizeof(IMAGE_NT_HEADERS),
		&FileOffset,
		NULL);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Read IMAGE_NT_HEADERS Fail:%X", status));
		ZwClose(hFile);
		return status;
	}
	pImageSectionHeader = ExAllocatePool(NonPagedPool, sizeof(IMAGE_SECTION_HEADER)*ImageNtHeaders.FileHeader.NumberOfSections);
	if (pImageSectionHeader==0)
	{
		KdPrint(("Allocate ImageSectionHeader Fail:%X", status));
		ZwClose(hFile);
		return status;
	}
	FileOffset.QuadPart += sizeof(IMAGE_NT_HEADERS);
	status = ZwReadFile(hFile,
		NULL,
		NULL,
		NULL,
		&ioStatusBlock,
		pImageSectionHeader,
		sizeof(IMAGE_SECTION_HEADER)*ImageNtHeaders.FileHeader.NumberOfSections,
		&FileOffset,
		NULL);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("Read pImageSectionHeader Fail:%X", status));
		ExFreePool(pImageSectionHeader);
		ZwClose(hFile);
		return status;
	}
	lpVirtualPointer = ExAllocatePool(NonPagedPool, ImageNtHeaders.OptionalHeader.SizeOfImage);
	if (lpVirtualPointer == 0)
	{
		KdPrint(("Allocate lpVirtualPointer is null"));
		ExFreePool(pImageSectionHeader);
		ZwClose(hFile);
		return status;
	}
	RtlZeroMemory(lpVirtualPointer, ImageNtHeaders.OptionalHeader.SizeOfImage);
	RtlCopyMemory(lpVirtualPointer, 
		&ImageDosHeader, 
		sizeof(IMAGE_DOS_HEADER));
	RtlCopyMemory((PVOID)((ULONG_PTR)lpVirtualPointer + ImageDosHeader.e_lfanew),
		&ImageNtHeaders, 
		sizeof(IMAGE_NT_HEADERS));
	RtlCopyMemory((PVOID)((ULONG_PTR)lpVirtualPointer + ImageDosHeader.e_lfanew+sizeof(IMAGE_NT_HEADERS)),
		pImageSectionHeader,
		sizeof(IMAGE_SECTION_HEADER)*ImageNtHeaders.FileHeader.NumberOfSections);
	for (uIndex = 0; uIndex < ImageNtHeaders.FileHeader.NumberOfSections; uIndex++)
	{
		SecVirtualAddress = pImageSectionHeader[uIndex].VirtualAddress;
		SizeOfSection = __Max(pImageSectionHeader[uIndex].SizeOfRawData,
			pImageSectionHeader[uIndex].Misc.VirtualSize);
		PointerToRawData = pImageSectionHeader[uIndex].PointerToRawData;
		FileOffset.QuadPart = PointerToRawData;
		status = ZwReadFile(hFile,
			NULL,
			NULL,
			NULL,
			&ioStatusBlock,
			(PVOID)((ULONG_PTR)lpVirtualPointer + SecVirtualAddress),
			SizeOfSection,
			&FileOffset,
			NULL);
		if (!NT_SUCCESS(status))
		{
			KdPrint(("Read Fail is pImageSectionHeader[%d]", uIndex));
			ExFreePool(pImageSectionHeader);
			ExFreePool(lpVirtualPointer);
			ZwClose(hFile);
			return status;
		}
	}
	ReLocMoudle(lpVirtualPointer,pOrigImage);
	SetNewSSDT(lpVirtualPointer, pOrigImage,&g_pNewServiceTable);
	KdPrint(("ok!"));
	ExFreePool(pImageSectionHeader);
	*lpVirtualAddress = lpVirtualPointer;
	ZwClose(hFile);
	return STATUS_SUCCESS;
}


PLDR_DATA_TABLE_ENTRY SearchDriver(PDRIVER_OBJECT pDriverObject,wchar_t *strDriverName)
{
	UNICODE_STRING				usDriverName;
	LDR_DATA_TABLE_ENTRY		*pDataTableEntry;
	PLIST_ENTRY					pList;
	pDataTableEntry = (LDR_DATA_TABLE_ENTRY*)pDriverObject->DriverSection;
	if (!pDataTableEntry)
	{
		return 0;
	}
	RtlInitUnicodeString(&usDriverName, strDriverName);
	pList = &pDataTableEntry->InLoadOrderLinks;
	do
	{
		UNICODE_STRING usBuffer;
		KdPrint(("%wZ", ((LDR_DATA_TABLE_ENTRY*)pList)->BaseDllName));
		if (0==RtlCompareUnicodeString(&usDriverName, &((LDR_DATA_TABLE_ENTRY*)pList)->BaseDllName, FALSE))
		{
			return (LDR_DATA_TABLE_ENTRY*)pList;
		}
		pList = pList->Blink;
	} while (pList->Blink != &pDataTableEntry->InLoadOrderLinks);
	return 0;
}

NTSTATUS DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	if (g_lpVirtualPointer)
	{
		ExFreePool(g_lpVirtualPointer);
		UnHookKiFastCallEntry(); 
	}
	KdPrint(("驱动卸载成功！"));
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING psRegPath)
{
	PLDR_DATA_TABLE_ENTRY pLdrDataTableEntry;
	pLdrDataTableEntry = SearchDriver(pDriverObject, L"ntoskrnl.exe");
	if (pLdrDataTableEntry)
	{
		KdPrint(("DllBase:%X", pLdrDataTableEntry->DllBase));
		ReadFileToMemory(L"\\??\\C:\\Windows\\System32\\ntkrnlpa.exe", &g_lpVirtualPointer, pLdrDataTableEntry->DllBase);
		g_NewKernelInc = (ULONG_PTR)g_lpVirtualPointer - (ULONG_PTR)pLdrDataTableEntry->DllBase;
		SearchKiFastCallEntry();
	}
	KdPrint(("g_lpVirtualPointer:%X", g_lpVirtualPointer));
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("驱动加载成功！"));
	return STATUS_SUCCESS;
}