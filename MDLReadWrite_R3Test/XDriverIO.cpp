#include "XDriverIO.h"




XDriverIO::XDriverIO(ULONG_PTR Pid):m_Pid(Pid)
{
	if (!this->isElevated()) {
		MessageBox(NULL, (LPWSTR)TEXT("请以管理员权限运行！"), (LPWSTR)TEXT("错误"), MB_OK);
		exit(1);
	}
	//删除sys
	DeleteFile(ExePath().append(L"\\XDriverIO.sys").c_str());
	//释放sys
	ReleaseRes(ExePath().append(L"\\XDriverIO.sys").c_str(), IDR_SYS1, L"sys");
	
	/*std::ifstream infile("XDriverIO.sys");
	if (!infile.good()) {
		std::ofstream fout;
		fout.open("XDriverIO.sys", std::ios::binary | std::ios::out);
		fout.write((char*)&XDriverIO::rawDriver, 20056);
		fout.close();
	}
	//关闭sys
	infile.close();*/

	MessageBox(NULL, (LPWSTR)TEXT("驱动安装成功！"), (LPWSTR)TEXT("提示"), MB_OK);
	//加载驱动
	LoadDriver(ExePath() + L"\\XDriverIO.sys", L"XDriverIO", L"Kernel level readprocessmemory and writeprocessmemory");
	m_InBuffer = malloc(200);
	RtlZeroMemory(m_InBuffer, 200);
	m_OutBuffer = malloc(200);
	RtlZeroMemory(m_OutBuffer, 200);
	InitDevice(m_Pid);
	//删除sys
	//DeleteFile(ExePath().append(L"\\XDriverIO.sys").c_str());
	//驱动删除sys
	if (ForceDelete(ExePath().append(L"\\XDriverIO.sys").c_str()))
	{
		std::cout << "success" << std::endl;
	}
	else
	{
		std::cout << "fail" << std::endl;
	}
	SetPid();
}

XDriverIO::~XDriverIO()
{
	if (!m_InBuffer)
	{
		free(m_InBuffer);
	}
	if (!m_OutBuffer)
	{
		m_OutBuffer;
	}
	CloseDevice(m_hDevice);
	StopService(L"XDriverIO");
}


BOOLEAN XDriverIO::InitDevice(ULONG_PTR Pid)
{
	m_hDevice = CreateFile(SymLink,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (m_hDevice == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	return TRUE;
}

VOID XDriverIO::CloseDevice(HANDLE hDevice)
{
	if (hDevice!=NULL)
	{
		CloseHandle(hDevice);
	}
}

VOID XDriverIO::SetPid()
{
	DWORD dwRet = 0;
	DeviceIoControl(m_hDevice, IOCTL_INIT, &m_Pid, sizeof(ULONG_PTR), NULL, 0, &dwRet, NULL);
}

VOID XDriverIO::ReadRaw(LPCVOID lpAddress, LPVOID lpBuffer, SIZE_T nSize)
{
	DWORD dwRet = 0;
	((PWRIO)m_InBuffer)->VirtualAddress = lpAddress;
	((PWRIO)m_InBuffer)->Length = nSize;
	DeviceIoControl(m_hDevice, IOCTL_READ, m_InBuffer, sizeof(WRIO), lpBuffer, (DWORD)nSize, &dwRet, (LPOVERLAPPED)NULL);
}

BOOLEAN XDriverIO::WriteRaw(LPCVOID lpAddress, LPVOID lpBuffer, SIZE_T nSize)
{
	DWORD dwRet = 0;
	((PWRIO)m_InBuffer)->VirtualAddress = lpAddress;
	((PWRIO)m_InBuffer)->Length = nSize;
	RtlCopyMemory((PVOID)((ULONG_PTR)m_InBuffer + sizeof(WRIO)), lpBuffer, nSize);
	DeviceIoControl(m_hDevice, IOCTL_WRITE, m_InBuffer, sizeof(WRIO) + (DWORD)nSize, NULL, (DWORD)nSize, &dwRet, (LPOVERLAPPED)NULL);
	if (dwRet == 0)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

HANDLE XDriverIO::GetMoudleHandle(LPCWSTR MoudleName)
{
	DWORD dwRet = 0;
	DeviceIoControl(m_hDevice, IOCTL_GMODHAN, (LPVOID)MoudleName, (DWORD)(wcslen(MoudleName) * sizeof(WCHAR)), m_OutBuffer, sizeof(ULONG_PTR), &dwRet, NULL);
	return *(HANDLE*)m_OutBuffer;
}

BOOLEAN XDriverIO::ForceDelete(LPCWSTR FilePath)
{
	DWORD dwRet = 0;
	DeviceIoControl(m_hDevice, IOCTL_FORCEDELETE, (LPVOID)FilePath, (DWORD)(wcslen(FilePath) * sizeof(WCHAR)), m_OutBuffer, sizeof(ULONG_PTR), &dwRet, NULL);
	if (dwRet==0)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}


bool XDriverIO::isElevated() {
	BOOL fRet = FALSE;
	HANDLE hToken = NULL;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
		TOKEN_ELEVATION Elevation;
		DWORD cbSize = sizeof(TOKEN_ELEVATION);
		if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
			fRet = Elevation.TokenIsElevated;
		}
	}
	if (hToken) {
		CloseHandle(hToken);
	}
	return fRet;
}




BOOL XDriverIO::LoadDriver(std::wstring TargetDriver, std::wstring TargetServiceName, std::wstring TargetServiceDesc)
{
	SC_HANDLE ServiceManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (!ServiceManager) return FALSE;
	SC_HANDLE ServiceHandle = CreateService(ServiceManager, (LPCWSTR)TargetServiceName.c_str(), (LPWSTR)TargetServiceDesc.c_str(), SERVICE_START | DELETE | SERVICE_STOP, SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START, SERVICE_ERROR_IGNORE, (LPWSTR)TargetDriver.c_str(), NULL, NULL, NULL, NULL, NULL);
	if (!ServiceHandle)
	{
		ServiceHandle = OpenService(ServiceManager, (LPCWSTR)TargetServiceName.c_str(), SERVICE_START | DELETE | SERVICE_STOP);
		if (!ServiceHandle) return FALSE;
	}
	if (!StartServiceA(ServiceHandle, NULL, NULL)) return FALSE;
	CloseServiceHandle(ServiceHandle);
	CloseServiceHandle(ServiceManager);
	return TRUE;
}

BOOL XDriverIO::StopService(std::wstring TargetServiceName)
{
	SERVICE_STATUS ServiceStatus;
	SC_HANDLE ServiceManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (!ServiceManager) return FALSE;
	SC_HANDLE ServiceHandle = OpenService(ServiceManager, (LPCWSTR)TargetServiceName.c_str(), SERVICE_STOP | DELETE);
	if (!ServiceHandle) return FALSE;
	if (!ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &ServiceStatus)) return FALSE;
	if (!DeleteService(ServiceHandle)) return FALSE;
	CloseServiceHandle(ServiceHandle);
	CloseServiceHandle(ServiceManager);
	return TRUE;
}

std::wstring XDriverIO::ExePath() {
	TCHAR exeFullPath[MAX_PATH]; // Full path  
	GetModuleFileName(NULL, exeFullPath, MAX_PATH);
	std::wstring strPath = __TEXT("");
	strPath = (std::wstring)exeFullPath;    // Get full path of the file  
	auto pos = strPath.find_last_of(L'\\', strPath.length());
	return strPath.substr(0, pos);  // Return the directory without the file name  
}
