#include "XDriverIO.h"
#include "stdio.h"
#include <locale>



int main()
{
	ULONG_PTR Pid = 0;
	printf("ÇëÊäÈëPid: ");
	scanf_s("%lld", &Pid);
 	XDriverIO* RPM = new XDriverIO(Pid);
	ULONG_PTR BaseAddress = (ULONG_PTR)RPM->GetMoudleHandle(L"user32.dll");
	auto orig = RPM->ReadMemory<ULONG_PTR>(BaseAddress);
	printf("%llX:%llX\n", BaseAddress, orig);
	RPM->WriteMemory<ULONG_PTR>(BaseAddress, 0x00111111);
	auto aa = RPM->ReadMemory<ULONG_PTR>(BaseAddress);
	printf("%llX:%llX\n", BaseAddress, aa);
	//auto a = RPM->ReadString<wchar_t>(0x1FABF8F1988,100);
	//std::wcout.imbue(std::locale("chs"));
	//std::wcout << a << std::endl;
	system("pause");
	RPM->WriteMemory<ULONG_PTR>(BaseAddress, orig);
	aa = RPM->ReadMemory<ULONG_PTR>(BaseAddress);
	printf("%llX:%llX\n", BaseAddress, aa);
	system("pause");
	delete RPM;
	return 0;
}

