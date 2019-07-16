#pragma once
#include <ntifs.h>


#define DEV_NAME L"\\Device\\MyDevice"
#define SYM_LINK_NAME L"\\??\\MySymLink"

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
}WRIO, *PWRIO;

KEVENT kEvent,kEvent2;

PVOID pIoBuffer;


#define DONT_HAVE_THREAD 0;
#define HAVE_THREAD 1
#define EXIT_THREAD 2

BOOLEAN bDieTherad = DONT_HAVE_THREAD;

NTKERNELAPI
UCHAR *
PsGetProcessImageFileName(
	__in PEPROCESS Process
);