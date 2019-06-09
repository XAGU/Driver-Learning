#include <ntifs.h>

#pragma pack(1) //写这个内存以一字节对齐 如果不写是以4字节的对齐的  
typedef struct ServiceDescriptorEntry {//这个结构就是为了管理这个数组而来的 内核api所在的数组 才有这个结构的 这个是ssdt  
	unsigned int *ServiceTableBase;//就是ServiceTable ssdt数组  
	unsigned int *ServiceCounterTableBase; //仅适用于checked build版本 无用  
	unsigned int NumberOfServices;//(ServiceTableBase)数组中有多少个元素 有多少个项  
	unsigned char *ParamTableBase;//参数表基址 我们层传过来的api的参数 占用多少字节 多大  
} ServiceDescriptorTableEntry_t, *PServiceDescriptorTableEntry_t;
#pragma pack()
__declspec(dllimport) ServiceDescriptorTableEntry_t KeServiceDescriptorTable;

typedef NTSTATUS(*fn_NtOpenProcess)(
	PHANDLE            ProcessHandle,
	ACCESS_MASK        DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PCLIENT_ID         ClientId
	);

//gloabe
ULONG_PTR g_Origin_NtOpenProcess;

//关闭页只读保护
void _declspec(naked) ShutPageProtect()
{
	__asm
	{
		push eax;
		mov eax, cr0;
		and eax, ~0x10000;
		mov cr0, eax;
		pop eax;
		ret;
	}
}

//开启页只读保护
void _declspec(naked) OpenPageProtect()
{
	__asm
	{
		push eax;
		mov eax, cr0;
		or eax, 0x10000;
		mov cr0, eax;
		pop eax;
		ret;
	}
}


BOOLEAN ProtectProcess(HANDLE pid)
{
	NTSTATUS status;
	PEPROCESS pEprocess_obj;
	status = PsLookupProcessByProcessId(pid, &pEprocess_obj);
	//KdPrint(("目标： %s", (CHAR*)pEprocess_obj + 0x16c));
	if (!NT_SUCCESS(status))
	{
		KdPrint(("--------------------------------%d", pid));
		return FALSE;
	}
	if (strstr((CHAR*)pEprocess_obj + 0x16c,"InstDrv.exe")!=NULL)
	{
		KdPrint(("--------------------------------%s", (CHAR*)pEprocess_obj + 0x16c));
		ObDereferenceObject(pEprocess_obj);
		return TRUE;
	}
	ObDereferenceObject(pEprocess_obj);
	return FALSE;
}

NTSTATUS NewNtOpenProcess(
	PHANDLE            ProcessHandle,
	ACCESS_MASK        DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PCLIENT_ID         ClientId
)
{
	if (ProtectProcess(ClientId->UniqueProcess))
	{
		DesiredAccess = 0;
		//KdPrint(("调用： %s", (CHAR*)PsGetCurrentProcess() + 0x16c));
	}
	return ((fn_NtOpenProcess)g_Origin_NtOpenProcess)(ProcessHandle, DesiredAccess, ObjectAttributes, ClientId);
}

NTSTATUS HookOpenProcess()
{
	NTSTATUS status;
	status = STATUS_SUCCESS;
	ShutPageProtect();
	g_Origin_NtOpenProcess = KeServiceDescriptorTable.ServiceTableBase[190];
	KeServiceDescriptorTable.ServiceTableBase[190] = (unsigned int)NewNtOpenProcess;
	OpenPageProtect();
	return status;
}


void UnHookOpenProcess()
{
	ShutPageProtect();
	KeServiceDescriptorTable.ServiceTableBase[190] = (unsigned int)g_Origin_NtOpenProcess;
	OpenPageProtect();
}

VOID DriverUnload(PDRIVER_OBJECT pDriver)
{
	KdPrint(("驱动卸载成功!"));
	UnHookOpenProcess();
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pRegPath)
{
	KdPrint(("驱动启动成功!"));
	HookOpenProcess();
	pDriver->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}


//#include <ntifs.h>
//
////内核之SSDT-HOOK
////系统服务表
//typedef struct _KSYSTEM_SERVICE_TABLE
//{
//	PULONG ServiceTableBase;       //函数地址表的首地址
//	PULONG ServiceCounterTableBase;//函数表中每个函数被调用的次数
//	ULONG  NumberOfService;        //服务函数的个数
//	ULONG ParamTableBase;          //参数个数表首地址
//}KSYSTEM_SERVICE_TABLE;
//
////服务描述符
//typedef struct _KSERVICE_TABLE_DESCRIPTOR
//{
//	KSYSTEM_SERVICE_TABLE ntoskrnl;//ntoskrnl.exe的服务函数,SSDT
//	KSYSTEM_SERVICE_TABLE win32k;  //win32k.sys的服务函数,ShadowSSDT
//	KSYSTEM_SERVICE_TABLE notUsed1;//暂时没用1
//	KSYSTEM_SERVICE_TABLE notUsed2;//暂时没用2
//}KSERVICE_TABLE_DESCRIPTOR;
//
////定义HOOK的函数的类型
//typedef NTSTATUS(NTAPI*FuZwOpenProcess)(
//	_Out_ PHANDLE ProcessHandle,
//	_In_ ACCESS_MASK DesiredAccess,
//	_In_ POBJECT_ATTRIBUTES ObjectAttributes,
//	_In_opt_ PCLIENT_ID ClientId
//	);
//
////自写的函数声明
//NTSTATUS NTAPI MyZwOpenProcess(
//	_Out_ PHANDLE ProcessHandle,
//	_In_ ACCESS_MASK DesiredAccess,
//	_In_ POBJECT_ATTRIBUTES ObjectAttributes,
//	_In_opt_ PCLIENT_ID ClientId
//);
//
////记录系统的该函数
//FuZwOpenProcess g_OldZwOpenProcess;
////服务描述符表指针
//KSERVICE_TABLE_DESCRIPTOR* g_pServiceTable = NULL;
////要保护进程的ID
//ULONG g_Pid = 1000;
//
////安装钩子
//void InstallHook();
////卸载钩子
//void UninstallHook();
////关闭页写入保护
//void ShutPageProtect();
////开启页写入保护
//void OpenPageProtect();
//
////卸载驱动
//void OutLoad(DRIVER_OBJECT* obj);
//
//
//
//////***驱动入口主函数***/
//NTSTATUS DriverEntry(DRIVER_OBJECT* driver, UNICODE_STRING* path)
//{
//	path;
//	KdPrint(("驱动启动成功！\n"));
//	//DbgBreakPoint();
//
//	//安装钩子
//	InstallHook();
//
//	driver->DriverUnload = OutLoad;
//	return STATUS_SUCCESS;
//}
//
////卸载驱动
//void OutLoad(DRIVER_OBJECT* obj)
//{
//	obj;
//	//卸载钩子
//	UninstallHook();
//}
//
////安装钩子
//void InstallHook()
//{
//	//1.获取KTHREAD
//	PETHREAD pNowThread = PsGetCurrentThread();
//	//2.获取ServiceTable表
//	g_pServiceTable = (KSERVICE_TABLE_DESCRIPTOR*)
//		(*(ULONG*)((ULONG)pNowThread + 0xbc));
//	//3.保存旧的函数
//	g_OldZwOpenProcess = (FuZwOpenProcess)
//		g_pServiceTable->ntoskrnl.ServiceTableBase[0xbe];
//	//4.关闭页只读保护
//	ShutPageProtect();
//	//5.写入自己的函数到SSDT表内
//	g_pServiceTable->ntoskrnl.ServiceTableBase[0xbe]
//		= (ULONG)MyZwOpenProcess;
//	//6.开启页只读保护
//	OpenPageProtect();
//}
//
////卸载钩子
//void UninstallHook()
//{
//	//1.关闭页只读保护
//	ShutPageProtect();
//	//2.写入原来的函数到SSDT表内
//	g_pServiceTable->ntoskrnl.ServiceTableBase[0xbe]
//		= (ULONG)g_OldZwOpenProcess;
//	//3.开启页只读保护
//	OpenPageProtect();
//}
//
////关闭页只读保护
//void _declspec(naked) ShutPageProtect()
//{
//	__asm
//	{
//		push eax;
//		mov eax, cr0;
//		and eax, ~0x10000;
//		mov cr0, eax;
//		pop eax;
//		ret;
//	}
//}
//
////开启页只读保护
//void _declspec(naked) OpenPageProtect()
//{
//	__asm
//	{
//		push eax;
//		mov eax, cr0;
//		or eax, 0x10000;
//		mov cr0, eax;
//		pop eax;
//		ret;
//	}
//}
//
////自写的函数
//NTSTATUS NTAPI MyZwOpenProcess(
//	_Out_ PHANDLE ProcessHandle,
//	_In_ ACCESS_MASK DesiredAccess,
//	_In_ POBJECT_ATTRIBUTES ObjectAttributes,
//	_In_opt_ PCLIENT_ID ClientId
//)
//{
//	//当此进程为要保护的进程时
//	if (ClientId->UniqueProcess == (HANDLE)g_Pid)
//	{
//		//设为拒绝访问
//		DesiredAccess = 0;
//	}
//	//调用原函数
//	return g_OldZwOpenProcess(
//		ProcessHandle,
//		DesiredAccess,
//		ObjectAttributes,
//		ClientId);
//}