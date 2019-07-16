#include <ntifs.h>


//int Number = 1000;
//
//VOID ThreadProc1(IN PVOID lpParameter)
//{
//	PKMUTEX pkMutex = (PKMUTEX)lpParameter;
//	while (TRUE)
//	{
//		KeWaitForSingleObject(pkMutex, Executive, KernelMode, FALSE, NULL);
//		if (Number > 0)
//		{
//			KeStallExecutionProcessor(1000);
//			DbgPrint("ThreadProc1 Run %d\n", Number--);
//		}
//		else
//		{
//			KeReleaseMutex(pkMutex, FALSE);
//			break;
//		}
//		KeReleaseMutex(pkMutex, FALSE);
//	}
//
//	DbgPrint("线程1执行完毕\n");
//	//结束线程
//	PsTerminateSystemThread(STATUS_SUCCESS);
//	return;
//}
//VOID ThreadProc2(IN PVOID lpParameter)
//{
//	PKMUTEX pkMutex = (PKMUTEX)lpParameter;
//	while (TRUE)
//	{
//		KeWaitForSingleObject(pkMutex, Executive, KernelMode, FALSE, NULL);
//		if (Number > 0)
//		{
//			KeStallExecutionProcessor(1000);
//			DbgPrint("ThreadProc2 Run %d\n", Number--);
//		}
//		else
//		{
//			KeReleaseMutex(pkMutex, FALSE);
//			break;
//		}
//		KeReleaseMutex(pkMutex, FALSE);
//	}
//
//	DbgPrint("线程2执行完毕\n");
//	//结束线程
//	PsTerminateSystemThread(STATUS_SUCCESS);
//	return;
//}
//
//VOID Test(void)
//{
//	HANDLE hMyThread1, hMyThread2;
//	KMUTEX kMutex;
//
//	//初始化内核互斥体
//	KeInitializeMutex(&kMutex, 0);
//
//	//创建系统线程
//	PsCreateSystemThread(&hMyThread1, 0, NULL, NtCurrentProcess(), NULL, ThreadProc1, &kMutex);
//	PsCreateSystemThread(&hMyThread2, 0, NULL, NtCurrentProcess(), NULL, ThreadProc2, &kMutex);
//
//	PVOID Pointer_Array[2] = { 0 };
//	//得到对象指针
//	ObReferenceObjectByHandle(hMyThread1, 0, NULL, KernelMode, &Pointer_Array[0], NULL);
//	ObReferenceObjectByHandle(hMyThread2, 0, NULL, KernelMode, &Pointer_Array[1], NULL);
//
//	//等待多个事件
//	KeWaitForMultipleObjects(2, Pointer_Array, WaitAll, Executive, KernelMode, FALSE, NULL, NULL);
//
//	//减少引用计数
//	ObDereferenceObject(Pointer_Array[0]);
//	ObDereferenceObject(Pointer_Array[1]);
//	DbgPrint("Test函数执行完毕\n");
//	return;
//}
//
//
//VOID DriverUnLoad(PDRIVER_OBJECT pDriverObject)
//{
//	KdPrint(("驱动加载成功！"));
//}
//
//DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
//{
//	Test();
//	pDriverObject->DriverUnload = DriverUnLoad;
//	KdPrint(("驱动加载成功！"));
//	return STATUS_SUCCESS;
//}


int Number = 100;
int Number2 = 100;

VOID ThreadProc1(IN PVOID lpParameter)
{
	PKEVENT pkEvent = (PKEVENT)lpParameter;
	while (TRUE)
	{
		if (Number > 0)
		{
			KeStallExecutionProcessor(1000);
			DbgPrint("ThreadProc1 Run %d\n", Number--);
		}
		else
		{
			//KeReleaseMutex(pkEvent, FALSE);
			break;
		}
		//KeReleaseMutex(pkEvent, FALSE);
	}

	DbgPrint("线程1执行完毕\n");
	KeSetEvent(pkEvent, NULL, FALSE);
	//结束线程
	PsTerminateSystemThread(STATUS_SUCCESS);
	return;
}
VOID ThreadProc2(IN PVOID lpParameter)
{
	PKEVENT kEvent = (PKEVENT)lpParameter;
	while (TRUE)
	{
		KeWaitForSingleObject(kEvent, Executive, KernelMode, FALSE, NULL);
		if (Number2 > 0)
		{
			KeStallExecutionProcessor(1000);
			DbgPrint("ThreadProc2 Run %d\n", Number2--);
		}
		else
		{
			//KeReleaseMutex(kEvent, FALSE);
			break;
		}
	//	KeReleaseMutex(kEvent, FALSE);
	}

	DbgPrint("线程2执行完毕\n");
	//结束线程
	PsTerminateSystemThread(STATUS_SUCCESS);
	return;
}

VOID Test(void)
{
	HANDLE hMyThread1, hMyThread2;
	KEVENT kEvent;
	KeInitializeEvent(&kEvent, NotificationEvent, FALSE);

	//创建系统线程
	PsCreateSystemThread(&hMyThread1, 0, NULL, NtCurrentProcess(), NULL, ThreadProc1, &kEvent);
	PsCreateSystemThread(&hMyThread2, 0, NULL, NtCurrentProcess(), NULL, ThreadProc2, &kEvent);

	PVOID Pointer_Array[2] = { 0 };
	//得到对象指针
	ObReferenceObjectByHandle(hMyThread1, 0, NULL, KernelMode, &Pointer_Array[0], NULL);
	ObReferenceObjectByHandle(hMyThread2, 0, NULL, KernelMode, &Pointer_Array[1], NULL);

	//等待多个事件
	KeWaitForMultipleObjects(2, Pointer_Array, WaitAll, Executive, KernelMode, FALSE, NULL, NULL);

	//减少引用计数
	ObDereferenceObject(Pointer_Array[0]);
	ObDereferenceObject(Pointer_Array[1]);
	DbgPrint("Test函数执行完毕\n");
	return;
}


VOID DriverUnLoad(PDRIVER_OBJECT pDriverObject)
{
	KdPrint(("驱动加载成功！"));
}

DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegPath)
{
	Test();
	pDriverObject->DriverUnload = DriverUnLoad;
	KdPrint(("驱动加载成功！"));
	return STATUS_SUCCESS;
}