#include "ForceDelete.h"

//Çý¶¯Ç¿É¾
//from https://github.com/icewall/ForceDelete/blob/7c4ef89ce3030c29f5b8f214b427bccf1b24a1e5/utilities.c#L235
BOOLEAN ForceDelete(LPCWSTR path)
{
	HANDLE fileHandle;
	NTSTATUS Status;
	IO_STATUS_BLOCK ioBlock;
	DEVICE_OBJECT *device_object = NULL;
	void* object = NULL;
	OBJECT_ATTRIBUTES fileObject;
	UNICODE_STRING uPath;

	PEPROCESS eproc = IoGetCurrentProcess();
	//switch context to UserMode
	KeAttachProcess(eproc);

	RtlInitUnicodeString(&uPath, path);

	InitializeObjectAttributes(&fileObject,
		&uPath,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	Status = IoCreateFileSpecifyDeviceObjectHint(
		&fileHandle,
		SYNCHRONIZE | FILE_WRITE_ATTRIBUTES | FILE_READ_ATTRIBUTES | FILE_READ_DATA, //0x100181 
		&fileObject,
		&ioBlock,
		0,
		0,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, //FILE_SHARE_VALID_FLAGS,
		FILE_OPEN,
		FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,//0x60,
		0,
		0,
		CreateFileTypeNone,
		0,
		IO_IGNORE_SHARE_ACCESS_CHECK,
		device_object);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("error in IoCreateFileSpecifyDeviceObjectHint");
		goto _Error;
	}

	Status = ObReferenceObjectByHandle(fileHandle, 0, 0, 0, &object, 0);

	if (!NT_SUCCESS(Status))
	{
		DbgPrint("error in ObReferenceObjectByHandle");
		ZwClose(fileHandle);
		goto _Error;
	}

	/*
	METHOD 1
	*/
	((FILE_OBJECT*)object)->SectionObjectPointer->ImageSectionObject = 0;
	((FILE_OBJECT*)object)->DeleteAccess = 1;

	ObDereferenceObject(object);
	ZwClose(fileHandle);

	Status = ZwDeleteFile(&fileObject);
	if (NT_SUCCESS(Status))
	{
		KeDetachProcess();
		return TRUE;
	}
_Error:
	KeDetachProcess();
	return FALSE;
}

