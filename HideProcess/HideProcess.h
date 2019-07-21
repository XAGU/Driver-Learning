#pragma once
#include "Common.h"



PLIST_ENTRY pList;

BOOLEAN AddEprocess(PEPROCESS pEProcess);
BOOLEAN RemoveEprocess(PEPROCESS pEProcess);
NTSTATUS HideProcess(HANDLE ProcessId);
NTSTATUS UnHideProcess(HANDLE ProcessId);