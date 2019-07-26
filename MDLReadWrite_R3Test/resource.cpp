#include "resource.h"

BOOL ReleaseRes(LPCTSTR szDLLFullPath, UINT uResID, LPCTSTR szResType)
{
	if (uResID <= 0 || !szResType)
	{
		return FALSE;
	}

	HRSRC hRsrc = FindResource(NULL, MAKEINTRESOURCE(uResID), szResType);
	if (NULL == hRsrc)
	{
		return FALSE;
	}

	DWORD dwSize = SizeofResource(NULL, hRsrc);
	if (dwSize <= 0)
	{
		return FALSE;
	}

	HGLOBAL hGlobal = LoadResource(NULL, hRsrc);
	if (NULL == hGlobal)
	{
		return FALSE;
	}

	LPVOID pBuffer = LockResource(hGlobal);
	if (NULL == pBuffer)
	{
		return FALSE;
	}

	HANDLE hFile = CreateFile(szDLLFullPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (INVALID_HANDLE_VALUE == hFile)
	{
		return FALSE;
	}


	DWORD dwWrited = 0;
	if (FALSE == WriteFile(hFile, pBuffer, dwSize, &dwWrited, NULL))
	{
		return FALSE;
	}

	CloseHandle(hFile);
	UnlockResource(hGlobal);
	FreeResource(hGlobal);
	return TRUE;
}
