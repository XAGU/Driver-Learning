#pragma once
#include "Common.h"

NTSTATUS ReadProcessMemory(PVOID VirtualAddress, SIZE_T Length, PVOID pIoBuffer);
NTSTATUS WriteProcessMemory(PVOID VirtualAddress, SIZE_T Length, PVOID pIoBuffer);
ULONG_PTR GetMoudleHandle(LPCWSTR ModuleName);