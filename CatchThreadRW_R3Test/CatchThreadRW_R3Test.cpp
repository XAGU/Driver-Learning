
#include "pch.h"
#include <iostream>
#include <windows.h>
#include <winioctl.h>


#define IOCTL_BASE 0x800
#define IOCTL_CODE(i) CTL_CODE(FILE_DEVICE_UNKNOWN,IOCTL_BASE+i,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_READ IOCTL_CODE(1)
#define IOCTL_WRITE IOCTL_CODE(2)
#define IOCTL_GETPID IOCTL_CODE(3)


typedef struct WRIO
{
	ULONG_PTR	Address;
	BOOLEAN		IsRead;
	size_t		Length;
	BOOLEAN		Status;
}WRIO;

int main()
{
	DWORD dwRet = 0;
	WRIO InputBuffer = {0};
	UCHAR OutputBuffer[128] = {0};
	BOOL bRet;
	while (true)
	{
		InputBuffer.IsRead = TRUE;
		printf("请输入要读取的地址 ：");
		scanf_s("%I64X", &InputBuffer.Address);
		printf("请输入要读取的大小 ：");
		scanf_s("%X", &InputBuffer.Length);
		HANDLE hDevice = CreateFile(L"\\\\.\\MySymLink",
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);
		if (hDevice == INVALID_HANDLE_VALUE)
		{
			printf("CreateFile Failed !");
			return 1;
		}
		//读取数据
		DWORD_PTR start = GetTickCount();
		for (size_t i = 0; i < 1000; i++)
		{
			//InputBuffer.Address += i*2;
			bRet = DeviceIoControl(hDevice, IOCTL_READ, &InputBuffer, sizeof(WRIO), OutputBuffer, InputBuffer.Length, &dwRet, NULL);
			//printf("%X ", *(UCHAR*)OutputBuffer);
		}
		DWORD_PTR  end = GetTickCount();
		printf("GetTickCount: %d\n", end - start);
		//bRet = DeviceIoControl(hDevice, IOCTL_READ, &InputBuffer, sizeof(WRIO), OutputBuffer, InputBuffer.Length, &dwRet, NULL);
		if (bRet)
		{
			printf("OutPut buffer:%d Bytes !\n", dwRet);
			if (dwRet <= 4)
			{
				printf("ULONG_PTR : %X\n", *(ULONG_PTR*)OutputBuffer);
				printf("ULONG_PTR : %I64X\n", *(ULONG_PTR*)OutputBuffer);
				printf("Float : %F\n", *(FLOAT*)OutputBuffer);
				printf("Double : %F\n", *(DOUBLE*)OutputBuffer);
			}
			else
			{
				for (size_t i = 0; i < dwRet; i++)
				{
					printf("字节 : %X\n", OutputBuffer[i]);
				}
			}
		}
		CloseHandle(hDevice);
	}
}


