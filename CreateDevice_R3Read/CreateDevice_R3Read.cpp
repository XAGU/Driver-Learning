#include "pch.h"
#include <iostream>

using namespace std;

int main()
{
	HANDLE		hDevice;
	hDevice = CreateFile(L"\\\\.\\MySymLink",
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hDevice==INVALID_HANDLE_VALUE)
	{
		cout << "CreateFile失败 ！ ERROR:"<<GetLastError()<<endl;
		return 1;
	}
	UCHAR buffer[10];
	ULONG ulRead;
	BOOL bRet;
	bRet = ReadFile(hDevice, buffer, 10, &ulRead, NULL);
	if (bRet)
	{
		cout << "Read " << ulRead << "Bytes" << endl;
		for (size_t i = 0; i < (size_t)ulRead; i++)
		{
			wcout << hex << buffer[i] << endl;
			//printf("%02X", buffer[i]);
		}
	}
	CloseHandle(hDevice);
	return 0;
}

