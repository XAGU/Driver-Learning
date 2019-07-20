#pragma once
#include "Common.h"

PVOID GetProcAddress(WCHAR *FuncName);
MIPROCESSLOADERENTRY GetMiProcessLoaderEntry();
NTSTATUS GetDriverObjectByName(PDRIVER_OBJECT *DriverObject, WCHAR *DriverName);
VOID InitInLoadOrderLinks(PLDR_DATA_TABLE_ENTRY LdrEntry);
VOID HideDriver(PDRIVER_OBJECT DriverObject, PVOID Context, ULONG Count);
NTSTATUS HidePCHDriverDepsSelf(PDRIVER_OBJECT pDriverObject);
