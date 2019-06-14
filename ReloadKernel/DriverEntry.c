#include <ntifs.h>
#include "ntimage.h"
#include "DriverEntry.h"

#define __Max(a,b) a>b?a:b

//global
PVOID g_lpVirtualPointer;

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
		uTypeOffsetArraySize = pImageBaseRelocation->SizeOfBlock - 
			sizeof(ULONG_PTR)*2 /sizeof(USHORT);
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
	KdPrint(("ok!"));
	ExFreePool(pImageSectionHeader);
	*lpVirtualAddress = lpVirtualPointer;
	ZwClose(hFile);
	return STATUS_SUCCESS;
}

NTSTATUS DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	if (g_lpVirtualPointer)
	{
		ExFreePool(g_lpVirtualPointer);
	}
	KdPrint(("驱动卸载成功！"));
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING psRegPath)
{
	ReadFileToMemory(L"\\??\\C:\\Windows\\System32\\ntkrnlpa.exe",&g_lpVirtualPointer,0x83C4F000);
	KdPrint(("g_lpVirtualPointer:%X", g_lpVirtualPointer));
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("驱动加载成功！"));
	return STATUS_SUCCESS;
}