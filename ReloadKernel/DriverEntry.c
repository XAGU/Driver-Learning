#include <ntifs.h>
#include "ntimage.h"
#include "DriverEntry.h"

#define __Max(a,b) a>b?a:b

NTSTATUS ReLocMoudle(PVOID pNewImage, PVOID pOrigImage)
{
	IMAGE_DATA_DIRECTORY	ImageDataDirectory;
	IMAGE_DOS_HEADER		ImageDosHeader;
	IMAGE_NT_HEADERS		ImageNtHeaders;
	ImageDosHeader = (PIMAGE_DOS_HEADER)pNewImage;
	return STATUS_SUCCESS;
}

NTSTATUS ReadFileToMemory(wchar_t *strFileName,PVOID *lpVirtualAddress)
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
		KdPrint(("ZwCreateFile:%X", status));
		return status;
	}

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
	FileOffset.QuadPart += sizeof(ImageNtHeaders);
	status = ZwReadFile(hFile,
		NULL,
		NULL,
		NULL,
		&ioStatusBlock,
		&pImageSectionHeader,
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
		KdPrint(("Allocate lpVirtualPointer Fail:%X", status));
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
		PointerToRawData = pImageSectionHeader[uIndex].PointerToRawData;
		SizeOfSection = __Max(pImageSectionHeader[uIndex].SizeOfRawData,
			pImageSectionHeader[uIndex].Misc.VirtualSize);
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
		if (NT_SUCCESS(status))
		{
			KdPrint(("Read Fail is pImageSectionHeader[%d]", uIndex));
			ExFreePool(pImageSectionHeader);
			ExFreePool(lpVirtualPointer);
			ZwClose(hFile);
			return status;
		}
	}

	ExFreePool(pImageSectionHeader);
	ExFreePool(lpVirtualAddress);
	*lpVirtualAddress = lpVirtualPointer;
	ZwClose(hFile);
	return STATUS_SUCCESS;
}

NTSTATUS DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("驱动卸载成功！"));
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING psRegPath)
{
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("驱动加载成功！"));
	return STATUS_SUCCESS;
}