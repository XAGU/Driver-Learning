//

#include "pch.h"
#include <windows.h>
#include <stdio.h>
#include <iostream>

#define __Max(a,b) a>b?a:b

void ReadPEFile()
{
	HANDLE hFile;
	ULONG uIndex;
	BOOL bStatus;
	DWORD dwRetSize;
	LARGE_INTEGER FileOffset;
	IMAGE_DOS_HEADER ImageDosHeader;
	IMAGE_NT_HEADERS ImageNtHeaders;
	IMAGE_SECTION_HEADER *pImageSectionHeader;
	hFile = CreateFile(L"D:\\桌面\\Read_Write_Test.exe",
		FILE_ALL_ACCESS,
		0,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("打开文件失败！ %d\n", GetLastError());
		return ;
	}
	bStatus = ReadFile(hFile, &ImageDosHeader, sizeof(IMAGE_DOS_HEADER), &dwRetSize, NULL);
	if (!bStatus)
	{
		printf("Read ImageDosHeader Failed %d", GetLastError());
		CloseHandle(hFile);
		return ;
	}
	printf("ImageDosHeader.e_magic:%s\nImageDosHeader.e_lfanew:%X\n",
		&ImageDosHeader.e_magic,
		ImageDosHeader.e_lfanew);
	FileOffset.QuadPart = ImageDosHeader.e_lfanew;
	bStatus = SetFilePointerEx(hFile, FileOffset, NULL, FILE_BEGIN);
	if (!bStatus)
	{
		printf("SetFilePointer Failed %d", GetLastError());
		CloseHandle(hFile);
		return ;
	}

	bStatus = ReadFile(hFile, &ImageNtHeaders, sizeof(IMAGE_NT_HEADERS), &dwRetSize, NULL);
	if (!bStatus)
	{
		printf("Read ImageNtHeaders Failed %d", GetLastError());
		CloseHandle(hFile);
		return ;
	}

	printf("ImageNtHeaders.Signature:%s\nNumberOfSections:%d\n",
		&ImageNtHeaders.Signature, ImageNtHeaders.FileHeader.NumberOfSections);

	FileOffset.QuadPart += sizeof(IMAGE_NT_HEADERS);
	bStatus = SetFilePointerEx(hFile, FileOffset, NULL, FILE_BEGIN);
	if (!bStatus)
	{
		printf("SetFilePointer Failed %d", GetLastError());
		CloseHandle(hFile);
		return ;
	}

	pImageSectionHeader = (IMAGE_SECTION_HEADER*)malloc(sizeof(IMAGE_SECTION_HEADER)*ImageNtHeaders.FileHeader.NumberOfSections);
	if (pImageSectionHeader == 0)
	{
		CloseHandle(hFile);
		return ;
	}

	bStatus = ReadFile(hFile,
		pImageSectionHeader,
		sizeof(IMAGE_SECTION_HEADER)*ImageNtHeaders.FileHeader.NumberOfSections,
		&dwRetSize,
		NULL
	);
	if (!bStatus)
	{
		printf("Read IMAGE_SECTION_HEADER Failed %d", GetLastError());
		free(pImageSectionHeader);
		CloseHandle(hFile);
		return ;
	}
	for (uIndex = 0; uIndex < ImageNtHeaders.FileHeader.NumberOfSections; uIndex++)
	{
		printf("pImageSectionHeader[%d]:%s\n", uIndex, &pImageSectionHeader[uIndex].Name);
	}
	free(pImageSectionHeader);
	CloseHandle(hFile);
	return;
}

void ReadFileToMemory()
{
	HANDLE hFile;
	ULONG uIndex;
	BOOL bStatus;
	DWORD dwRetSize;
	DWORD VirtualSizeOfImage;
	LARGE_INTEGER FileOffset;
	IMAGE_DOS_HEADER ImageDosHeader;
	IMAGE_NT_HEADERS ImageNtHeaders;
	PVOID lpVirtualPointer;
	DWORD SecVirtualAddress,SizeOfSection;
	DWORD PointerToRawData;
	IMAGE_SECTION_HEADER *pImageSectionHeader;
	hFile = CreateFile(L"D:\\桌面\\Read_Write_Test.exe",
		FILE_ALL_ACCESS,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("打开文件失败！ %d\n", GetLastError());
		return;
	}
	bStatus = ReadFile(hFile, &ImageDosHeader, sizeof(IMAGE_DOS_HEADER), &dwRetSize, NULL);
	if (!bStatus)
	{
		printf("Read ImageDosHeader Failed %d", GetLastError());
		CloseHandle(hFile);
		return;
	}
	printf("ImageDosHeader.e_magic:%s\nImageDosHeader.e_lfanew:%X\n",
		&ImageDosHeader.e_magic,
		ImageDosHeader.e_lfanew);
	FileOffset.QuadPart = ImageDosHeader.e_lfanew;
	bStatus = SetFilePointerEx(hFile, FileOffset, NULL, FILE_BEGIN);
	if (!bStatus)
	{
		printf("SetFilePointer Failed %d", GetLastError());
		CloseHandle(hFile);
		return;
	}

	bStatus = ReadFile(hFile, &ImageNtHeaders, sizeof(IMAGE_NT_HEADERS), &dwRetSize, NULL);
	if (!bStatus)
	{
		printf("Read ImageNtHeaders Failed %d", GetLastError());
		CloseHandle(hFile);
		return;
	}

	printf("ImageNtHeaders.Signature:%s\nNumberOfSections:%d\n",
		&ImageNtHeaders.Signature, ImageNtHeaders.FileHeader.NumberOfSections);

	FileOffset.QuadPart += sizeof(IMAGE_NT_HEADERS);
	bStatus = SetFilePointerEx(hFile, FileOffset, NULL, FILE_BEGIN);
	if (!bStatus)
	{
		printf("SetFilePointer Failed %d", GetLastError());
		CloseHandle(hFile);
		return;
	}

	pImageSectionHeader = (IMAGE_SECTION_HEADER*)malloc(sizeof(IMAGE_SECTION_HEADER)*ImageNtHeaders.FileHeader.NumberOfSections);
	if (pImageSectionHeader == 0)
	{
		free(pImageSectionHeader);
		CloseHandle(hFile);
		return;
	}

	bStatus = ReadFile(hFile,
		pImageSectionHeader,
		sizeof(IMAGE_SECTION_HEADER)*ImageNtHeaders.FileHeader.NumberOfSections,
		&dwRetSize,
		NULL
	);
	if (!bStatus)
	{
		printf("Read IMAGE_SECTION_HEADER Failed %d", GetLastError());
		free(pImageSectionHeader);
		CloseHandle(hFile);
		return;
	}
	for (uIndex = 0; uIndex < ImageNtHeaders.FileHeader.NumberOfSections; uIndex++)
	{
		printf("pImageSectionHeader[%d]:%s\n", uIndex, &pImageSectionHeader[uIndex].Name);
	}

	VirtualSizeOfImage = ImageNtHeaders.OptionalHeader.SizeOfImage;
	lpVirtualPointer = malloc(VirtualSizeOfImage);
	if (lpVirtualPointer==0)
	{
		free(lpVirtualPointer);
		free(pImageSectionHeader);
		CloseHandle(hFile);
		return;
	}

	ZeroMemory(lpVirtualPointer, VirtualSizeOfImage);

	memcpy(lpVirtualPointer,
		&ImageDosHeader,
		sizeof(ImageDosHeader));
	memcpy((PVOID)((ULONG_PTR)lpVirtualPointer + ImageDosHeader.e_lfanew),
		&ImageNtHeaders, 
		sizeof(IMAGE_NT_HEADERS));
	memcpy((PVOID)((ULONG_PTR)lpVirtualPointer + ImageDosHeader.e_lfanew +sizeof(IMAGE_NT_HEADERS)),
		pImageSectionHeader, 
		sizeof(IMAGE_SECTION_HEADER) * ImageNtHeaders.FileHeader.NumberOfSections);
	
	for (uIndex = 0;  uIndex < ImageNtHeaders.FileHeader.NumberOfSections; uIndex++)
	{
		SecVirtualAddress = pImageSectionHeader[uIndex].VirtualAddress;
		PointerToRawData = pImageSectionHeader[uIndex].PointerToRawData;
		SizeOfSection =__Max(pImageSectionHeader[uIndex].SizeOfRawData, 
			pImageSectionHeader[uIndex].Misc.VirtualSize);
		FileOffset.QuadPart = PointerToRawData;
		SetFilePointerEx(hFile, FileOffset, NULL, FILE_BEGIN);
		if (!bStatus)
		{
			printf("SetFilePointer Failed %d", GetLastError());
			free(pImageSectionHeader);
			free(lpVirtualPointer);
			CloseHandle(hFile);
			return;
		}
		bStatus = ReadFile(hFile,
			(PVOID)((ULONG_PTR)lpVirtualPointer + SecVirtualAddress),
			SizeOfSection,
			&dwRetSize,
			NULL);
		if (!bStatus)
		{
			free(pImageSectionHeader);
			free(lpVirtualPointer);
			CloseHandle(hFile);
			return;
		}
	}
	printf("lpVirtualPointer:0x%llx", lpVirtualPointer);
	getchar();
	getchar();
	free(pImageSectionHeader);
	free(lpVirtualPointer);
	CloseHandle(hFile);
	return;
}

int main()
{
	ReadFileToMemory();
	return 1;
}