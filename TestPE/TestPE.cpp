//

#include "pch.h"
#include <windows.h>
#include <stdio.h>
#include <iostream>

int main()
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
		printf("打开文件失败！ %d\n",GetLastError());
		return 0;
	}
	bStatus = ReadFile(hFile, &ImageDosHeader, sizeof(IMAGE_DOS_HEADER), &dwRetSize, NULL);
	if (!bStatus)
	{
		printf("Read ImageDosHeader Failed %d", GetLastError());
		CloseHandle(hFile);
		return 0;
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
		return 0;
	}
	
	bStatus = ReadFile(hFile, &ImageNtHeaders, sizeof(IMAGE_NT_HEADERS), &dwRetSize, NULL);
	if (!bStatus)
	{
		printf("Read ImageNtHeaders Failed %d", GetLastError());
		CloseHandle(hFile);
		return 0;
	}

	printf("ImageNtHeaders.Signature:%s\nNumberOfSections:%d\n",
		&ImageNtHeaders.Signature,ImageNtHeaders.FileHeader.NumberOfSections);

	FileOffset.QuadPart+=sizeof(IMAGE_NT_HEADERS);
	bStatus = SetFilePointerEx(hFile, FileOffset, NULL, FILE_BEGIN);
	if (!bStatus)
	{
		printf("SetFilePointer Failed %d", GetLastError());
		CloseHandle(hFile);
		return 0;
	}

	pImageSectionHeader = (IMAGE_SECTION_HEADER*)malloc(sizeof(IMAGE_SECTION_HEADER)*ImageNtHeaders.FileHeader.NumberOfSections);
	if (pImageSectionHeader==0)
	{
		CloseHandle(hFile);
		return 0;
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
		CloseHandle(hFile);
		return 0;
	}
	for (uIndex = 0; uIndex < ImageNtHeaders.FileHeader.NumberOfSections; uIndex++)
	{
		printf("pImageSectionHeader[%d]:%s\n", uIndex, &pImageSectionHeader[uIndex].Name);
	}
	CloseHandle(hFile);
	getchar();
	return 1;
}