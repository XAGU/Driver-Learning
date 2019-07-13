// MyDriverIO_R3.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <windows.h>
#include <winioctl.h>


#define IOCTL_BASE 0x800
#define IOCTL_CODE(i) CTL_CODE(FILE_DEVICE_UNKNOWN,IOCTL_BASE+i,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_READ IOCTL_CODE(1)
#define IOCTL_WRITE IOCTL_CODE(2)
#define IOCTL_GETPID IOCTL_CODE(3)

int main()
{
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
	DWORD_PTR dwRet = 0;
	UCHAR InputBuffer[10];
	UCHAR OutputBuffer[10];
	memset(InputBuffer, 0x66, 10);
	memset(OutputBuffer, 0x55, 10);
	BOOL bRet;
	//读取数据
	bRet = DeviceIoControl(hDevice, IOCTL_READ, InputBuffer, 10, OutputBuffer, 10, &dwRet, NULL);
	if (bRet)
	{
		printf("OutPut buffer:%d Bytes !\n", dwRet);
		for (size_t i = 0; i < dwRet; i++)
		{
			printf("%X", OutputBuffer[i]);
		}
	}
	//写入数据
	DeviceIoControl(hDevice, IOCTL_WRITE, InputBuffer, 10, OutputBuffer, 10, &dwRet, NULL);

	//写入pid，驱动KIll进程
	DWORD Pid;
	printf("请输入进程Pid：");
	scanf_s("%d", &Pid);
	DeviceIoControl(hDevice, IOCTL_GETPID, &Pid, sizeof(DWORD), OutputBuffer, 10, &dwRet, NULL);
}

