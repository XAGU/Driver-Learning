#include "XDriverIO.h"
#include "stdio.h"
#include <locale>



int main()
{
	ULONG_PTR Pid = 0;
	printf("ÇëÊäÈëPid: ");
	scanf_s("%lld", &Pid);
 	XDriverIO* RPM = new XDriverIO(Pid);
	ULONG_PTR BaseAddress = (ULONG_PTR)RPM->GetMoudleHandle(L"aaa.exe");
	auto aa = RPM->ReadMemory<ULONG_PTR>(BaseAddress);
	//RPM->WriteMemory<ULONG_PTR>(BaseAddress, 0xABCDEF);
	printf("%llX", aa);
	auto a = RPM->ReadString<wchar_t>(0x1FABF8F1988,100);
	std::wcout.imbue(std::locale("chs"));
	std::wcout << a << std::endl;
	delete RPM;
	system("pause");
	return 0;
}

