#pragma once
#include <Windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include "resource.h"

#define IOCTL_BASE 0x800
#define IOCTL_CODE(i) CTL_CODE(FILE_DEVICE_UNKNOWN,IOCTL_BASE+i,METHOD_BUFFERED,FILE_ANY_ACCESS)

#define IOCTL_READ IOCTL_CODE(1)//读
#define IOCTL_WRITE IOCTL_CODE(2)//写
#define IOCTL_INIT IOCTL_CODE(3)//初始化
#define IOCTL_GMODHAN IOCTL_CODE(4)//取模块地址
#define IOCTL_FORCEDELETE IOCTL_CODE(5)//强删文件

typedef struct WRIO
{
	LPCVOID		VirtualAddress;
	size_t		Length;
} WRIO, *PWRIO;

class XDriverIO
{
private:
	//打开设备对象
	BOOLEAN InitDevice(ULONG_PTR Pid);
	//关闭设备对象
	VOID CloseDevice(HANDLE hDevice);
	//设置进程
	VOID SetPid();
	//读字节集
	VOID ReadRaw(LPCVOID lpAddress, LPVOID lpBuffer, SIZE_T nSize);
	//写字节集
	BOOLEAN WriteRaw(LPCVOID lpAddress, LPVOID lpBuffer, SIZE_T nSize);
	//判断是否为管理员权限
	bool isElevated();

	//加载驱动
	BOOL LoadDriver(std::wstring TargetDriver, std::wstring TargetServiceName, std::wstring TargetServiceDesc);
	//卸载驱动
	BOOL StopService(std::wstring TargetServiceName);
	//取当前路径
	std::wstring ExePath();

public:
	XDriverIO(ULONG_PTR Pid);
	~XDriverIO();
	//读内存
	template <typename T>
	T ReadMemory(ULONG_PTR Address);
	//写内存
	template <typename T>
	BOOLEAN WriteMemory(ULONG_PTR Address, T Value);
	//读字符串
	template<class CharT>
	std::basic_string<CharT> ReadString(DWORD_PTR address, size_t max_length);
	//取模块基址
	HANDLE GetMoudleHandle(LPCWSTR MoudleName);
	//强删文件
	BOOLEAN ForceDelete(LPCWSTR FilePath);


private:
	//输入缓冲区
	LPVOID  m_InBuffer;
	//输出缓冲区
	LPVOID  m_OutBuffer;
	HANDLE	m_hDevice;
	ULONG_PTR m_Pid = 0;
	const LPCWSTR SymLink = L"\\\\.\\MySymLink";
};

template<typename T>
inline T XDriverIO::ReadMemory(ULONG_PTR Address)
{
	this->ReadRaw((LPCVOID)Address, m_OutBuffer, sizeof(T));
	return *(T*)m_OutBuffer;
}

template<typename T>
inline BOOLEAN XDriverIO::WriteMemory(ULONG_PTR Address, T Value)
{
	return this->WriteRaw((LPCVOID)Address, &Value, sizeof(T));
}

template<class CharT>
inline std::basic_string<CharT> XDriverIO::ReadString(DWORD_PTR address, size_t max_length)
{
	std::basic_string<CharT> str(max_length, CharT());
	this->ReadRaw((LPVOID)address, &str[0], sizeof(CharT) * max_length);
	auto it = str.find(CharT());
	if (it == str.npos) str.clear();
	else str.resize(it);
	return str;
}
