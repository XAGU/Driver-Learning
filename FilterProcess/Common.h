#pragma once
#include <ntifs.h>
#include <ntimage.h>


NTKERNELAPI
PVOID
PsGetProcessSectionBaseAddress(
	__in PEPROCESS Process
);


UCHAR			ReadCode[5] = {0};
ULONG_PTR		Offset = 0;