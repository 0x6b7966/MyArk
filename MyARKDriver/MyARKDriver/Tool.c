#include "Tool.h"
#include <ntstrsafe.h>
#include <wdm.h>
#include "data.h"

NTKERNELAPI CHAR* PsGetProcessImageFileName(PEPROCESS proc);
NTKERNELAPI PPEB PsGetProcessPeb(PEPROCESS pRoc);
//*****************************************************************************************
// ��������: EnumDriver
// ����˵��: ��������
// ��    ��: lracker
// ʱ    ��: 2019/12/23
// ��    ��: VOID
// �� �� ֵ: ULONG �����ֽ���
//*****************************************************************************************
ULONG EnumDriver(PVOID pOutPut, ULONG ulBuffSize, PDRIVER_OBJECT pDriver)
{
	PLDR_DATA_TABLE_ENTRY pLdr = (PLDR_DATA_TABLE_ENTRY)pDriver->DriverSection;
	PLDR_DATA_TABLE_ENTRY pBegin = pLdr;
	PDRIVER pDriverHead = ExAllocatePool(PagedPool, sizeof(DRIVER));
	// memset(pDriverHead, 0, sizeof(DRIVER));
	RtlZeroMemory(pDriverHead, sizeof(DRIVER));
	pDriverHead->Next = NULL;
	PDRIVER Temp = pDriverHead;
	ULONG ulCount = 0;
	__try {
		do
		{
			RtlCopyMemory(Temp->Name, pLdr->BaseDllName.Buffer, pLdr->BaseDllName.Length);
			Temp->Size = pLdr->SizeOfImage;
			Temp->DllBase = (ULONG)pLdr->DllBase;
			RtlCopyMemory(Temp->FullDllName, pLdr->FullDllName.Buffer, pLdr->FullDllName.Length);
			pLdr = (PLDR_DATA_TABLE_ENTRY)(pLdr->InLoadOrderLinks.Flink);
			PDRIVER pEnumDrivertemp;
			pEnumDrivertemp = ExAllocatePool(PagedPool, sizeof(DRIVER));
			// memset(pEnumDrivertemp, 0, sizeof(DRIVER));
			RtlZeroMemory(pEnumDrivertemp, sizeof(DRIVER));
			Temp->Next = pEnumDrivertemp;
			pEnumDrivertemp->Next = NULL;
			Temp = pEnumDrivertemp;
			ulCount++;
		} while (pBegin != pLdr);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("Error: %08x", GetExceptionCode());
	}
	ULONG OutPutSize = ulCount * sizeof(DRIVER);
	// �����Ҫ�Ĵ�С�ȴ������Ĵ�С���򷵻���Ҫ�Ĵ�С
	PDRIVER OutDriver = pOutPut;
	if (OutPutSize <= ulBuffSize)
	{
		Temp = pDriverHead;
		for (ULONG i = 0; i < ulCount; ++i)
		{
			RtlCopyMemory(OutDriver[i].Name, Temp->Name, 256 * 2);
			OutDriver[i].DllBase = Temp->DllBase;
			OutDriver[i].Size = Temp->Size;
			RtlCopyMemory(OutDriver[i].FullDllName, Temp->FullDllName, 256 * 2);
			Temp = Temp->Next;
		}
	}
	// ����ռ�
	Temp = pDriverHead;
	for (ULONG i = 0; i <= ulCount; ++i)
	{
		ExFreePool(Temp);
		Temp = Temp->Next;
	}
	return OutPutSize;
}

//*****************************************************************************************
// ��������: HideDriver
// ����˵��: ��������
// ��    ��: lracker
// ʱ    ��: 2019/12/24
// ��    ��: UNICODE_STRING
// �� �� ֵ: BOOL �����Ƿ�ɹ�
//*****************************************************************************************
UINT8 HideDriver(UNICODE_STRING DriverName, PDRIVER_OBJECT pDriver)
{
	pDriver;
	PLDR_DATA_TABLE_ENTRY pLdr = (PLDR_DATA_TABLE_ENTRY)pDriver->DriverSection;
	PLDR_DATA_TABLE_ENTRY pBegin = pLdr;
	__try {
		do
		{
			// ���ƥ�䵽������ͬ�������
			if (!RtlCompareUnicodeString(&DriverName, &(pLdr->BaseDllName), FALSE))
			{
				DbgPrint("�ҵ���,��ʼ����\n");
				pLdr->InLoadOrderLinks.Blink->Flink = pLdr->InLoadOrderLinks.Flink;
				pLdr->InLoadOrderLinks.Flink->Blink = pLdr->InLoadOrderLinks.Blink;
				pLdr->InLoadOrderLinks.Blink = NULL;
				pLdr->InLoadOrderLinks.Flink = NULL;
				break;
			}
			pLdr = (PLDR_DATA_TABLE_ENTRY)(pLdr->InLoadOrderLinks.Flink);
		} while (pBegin != pLdr);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		DbgPrint("Error: %08x", GetExceptionCode());
	}
	return TRUE;
}

//*****************************************************************************************
// ��������: EnumProcess
// ����˵��: ��������
// ��    ��: lracker
// ʱ    ��: 2019/12/24
// ��    ��: PVOID
// ��    ��: ULONG
// ��    ��: PDRIVER_OBJECT
// �� �� ֵ: ULONG
//*****************************************************************************************
ULONG EnumProcess(PVOID pOutPut, ULONG ulBuffSize)
{
	NTSTATUS Status;
	Status = STATUS_SUCCESS;
	PPROCESS pProcessHead = ExAllocatePool(PagedPool, sizeof(PROCESS));
	// memset(pProcessHead, 0, sizeof(PROCESS));
	RtlZeroMemory(pProcessHead, sizeof(PROCESS));
	pProcessHead->Next = NULL;
	PPROCESS Temp;
	Temp = pProcessHead;
	ULONG ulCount;
	ulCount = 0;
	PEPROCESS pProc;
	pProc = NULL;
	for (int i = 4; i < 90000; i += 4)
	{
		// ���Ի�ȡ����EPROCESS
		Status = PsLookupProcessByProcessId((HANDLE)i, &pProc);
		// �ж��Ƿ�Ϊ������
		ULONG* pulObjectTable = (ULONG*)((ULONG)pProc + 0xf4);
		if (!NT_SUCCESS(Status) || !(*pulObjectTable))
			continue;
		__try
		{
			ULONG ulPid = (ULONG)PsGetProcessId(pProc);
			Temp->dwPID = ulPid;
			ULONG* PPID = (ULONG*)((ULONG)pProc + 0x140);
			ULONG ulPPid = *PPID;
			Temp->dwPPID = ulPPid;
			RtlCopyMemory(Temp->Name, PsGetProcessImageFileName(pProc), strlen(PsGetProcessImageFileName(pProc)));
			PCWSTR FILENAME = (PCWSTR)(*(ULONG*)((ULONG)pProc + 0x1ec) + 8);
			UNICODE_STRING uFILENAME;
			RtlInitUnicodeString(&uFILENAME, FILENAME);
			RtlCopyMemory(Temp->FullDllName, uFILENAME.Buffer, uFILENAME.Length);
			PPROCESS pProcessTemp = ExAllocatePool(PagedPool, sizeof(PROCESS));
			// memset(pProcessTemp, 0, sizeof(PROCESS));
			RtlZeroMemory(pProcessTemp, sizeof(PROCESS));
			Temp->Next = pProcessTemp;
			pProcessTemp->Next = NULL;
			Temp = pProcessTemp;
			++ulCount;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {}
		ObDereferenceObject(pProc);
	}
	ULONG OutPutSize = ulCount * sizeof(PROCESS);
	// �����Ҫ�Ĵ�С�ȴ������Ĵ�С���򷵻���Ҫ�Ĵ�С
	PPROCESS OutProcess = pOutPut;
	if (OutPutSize <= ulBuffSize)
	{
		Temp = pProcessHead;
		for (ULONG i = 0; i < ulCount; ++i)
		{
			RtlCopyMemory(OutProcess[i].Name, Temp->Name, 256);
			OutProcess[i].dwPID = Temp->dwPID;
			OutProcess[i].dwPPID = Temp->dwPPID;
			RtlCopyMemory(OutProcess[i].FullDllName, Temp->FullDllName, 256);
			Temp = Temp->Next;
		}
	}
	// ����ռ�
	Temp = pProcessHead;
	for (ULONG i = 0; i <= ulCount; ++i)
	{
		ExFreePool(Temp);
		Temp = Temp->Next;
	}
	return OutPutSize;
}

//*****************************************************************************************
// ��������: EnumProcess
// ����˵��: �����߳�
// ��    ��: lracker
// ʱ    ��: 2019/12/24
// ��    ��: PVOID
// ��    ��: ULONG
// ��    ��: PVOID
// �� �� ֵ: ULONG
//*****************************************************************************************
ULONG EnumThread(PVOID pOutPut, ULONG ulBuffSize, PVOID pPID)
{
	pOutPut;
	ulBuffSize;
	pPID;
	PETHREAD _thread;
	PKTHREAD _kthread;
	PLIST_ENTRY LClink;
	PLIST_ENTRY LNink;
	CLIENT_ID* _CLIENT_ID;
	char BasePriority;
	UCHAR State;
	ULONG ThreadID = 0;
	ULONG ThreadStartAddress;
	// ��ȡ����Ҫ��ѯ��PID
	ULONG dwInputPID;
	dwInputPID = *(ULONG*)pPID;
	NTSTATUS Status;
	Status = STATUS_SUCCESS;
	PEPROCESS pProc;
	ULONG ulCount;
	ulCount = 0;
	PTHREAD pThreadHead = ExAllocatePool(PagedPool, sizeof(THREAD));
	// memset(pThreadHead, 0, sizeof(THREAD));
	RtlZeroMemory(pThreadHead, sizeof(THREAD));
	pThreadHead->Next = NULL;
	PTHREAD pThreadTemp = pThreadHead;
	for (int i = 4; i < 90000; i += 4)
	{
		// ���Ի�ȡ����EPROCESS
		Status = PsLookupProcessByProcessId((HANDLE)i, &pProc);
		// �ж��Ƿ�Ϊ������
		ULONG* pulObjectTable = (ULONG*)((ULONG)pProc + 0xf4);
		if (!NT_SUCCESS(Status) || !(*pulObjectTable))
			continue;
		ULONG* pulPID = (ULONG*)((ULONG)pProc + 0xB4);
		ULONG ulPID = *pulPID;
		// �ж�PID�Ƿ���ͬ
		if(ulPID != dwInputPID)
			continue;
		// ��ʼ�����ý��̵��߳�
		LClink = (PLIST_ENTRY)((ULONG)pProc + 0x188);
		LNink = LClink->Flink;

		while (LNink!=LClink)
		{
			_thread = (PETHREAD)((ULONG)LNink - 0x268);
			_kthread = (PKTHREAD)_thread;
			_CLIENT_ID = (PCLIENT_ID)((ULONG)_thread + 0x22C);
			ThreadID = (ULONG)(_CLIENT_ID->UniqueThread);				// TID
			ThreadStartAddress = *(ULONG*)((ULONG)_thread + 0x260);		// StartAddress
			BasePriority = *(char*)((ULONG)_kthread + 0x57);			// BasePriority
			State = *(UCHAR*)((ULONG)_kthread + 0x68);					// State

			pThreadTemp->ulTID = ThreadID;
			pThreadTemp->ulStartAddress = ThreadStartAddress;
			pThreadTemp->ulBasePriority = BasePriority;
			pThreadTemp->ulStatus = State;

			PTHREAD pThreadTemp2 = ExAllocatePool(PagedPool, sizeof(THREAD));
			// memset(pThreadTemp2, 0, sizeof(THREAD));
			RtlZeroMemory(pThreadTemp2, sizeof(THREAD));
			pThreadTemp2->Next = NULL;
			pThreadTemp->Next = pThreadTemp2;
			pThreadTemp = pThreadTemp2;
			LNink = LNink->Flink;
			// �Ȼ�ȡ������߳�����
			ulCount++;
		}
		ObDereferenceObject(pProc);
		break;
	}
	ULONG OutPutSize = ulCount * sizeof(THREAD);
	// �����Ҫ�Ĵ�С�ȴ������Ĵ�С���򷵻���Ҫ�Ĵ�С
	PTHREAD OutThread = pOutPut;
	if (OutPutSize <= ulBuffSize)
	{
		pThreadTemp = pThreadHead;
		for (ULONG i = 0; i < ulCount; ++i)
		{
			OutThread[i].ulTID = pThreadTemp->ulTID;					// TID
			OutThread[i].ulStartAddress = pThreadTemp->ulStartAddress;	// StartAddress
			OutThread[i].ulStatus = pThreadTemp->ulStatus;				// Status
			OutThread[i].ulBasePriority = pThreadTemp->ulBasePriority;	// BasePriority
			pThreadTemp = pThreadTemp->Next;
		}
	}
	// ����ռ�
	pThreadTemp = pThreadHead;
	for (ULONG i = 0; i <= ulCount; ++i)
	{
		ExFreePool(pThreadTemp);
		pThreadTemp = pThreadTemp->Next;
	}
	return OutPutSize;
}

//*****************************************************************************************
// ��������: EnumModule
// ����˵��: ����ģ��
// ��    ��: lracker
// ʱ    ��: 2019/12/25
// ��    ��: PVOID
// ��    ��: ULONG
// ��    ��: PVOID
// �� �� ֵ: ULONG
//*****************************************************************************************
ULONG EnumModule(PVOID pOutPut, ULONG ulBuffSize, PVOID pPID)
{
	pOutPut;
	ulBuffSize;
	pPID;
	ULONG ulInputPID;
	NTSTATUS Status;
	PEPROCESS pProc;
	ULONG* pulObjectTable;
	ULONG ulCount;
	ULONG ulPID;
	PMODULE pModuleHead;
	PMODULE pModuleTemp2;
	PMODULE pModuleTemp;
	struct _PEB* pPeb;
	LDR_DATA_TABLE_ENTRY* pLdrHeader;
	LDR_DATA_TABLE_ENTRY* pBegin;
	ulCount = 0;
	ulInputPID = *(ULONG*)pPID;
	Status = STATUS_SUCCESS;
	pModuleHead = ExAllocatePool(PagedPool, sizeof(MODULE));
	// memset(pModuleHead, 0, sizeof(MODULE));
	RtlZeroMemory(pModuleHead, sizeof(MODULE));
	pModuleTemp = pModuleHead;
	for (int i = 4; i < 90000; i += 4)
	{
		// ���Ի�ȡ����EPROCESS
		Status = PsLookupProcessByProcessId((HANDLE)i, &pProc);
		// �ж��Ƿ�Ϊ������
		pulObjectTable = (ULONG*)((ULONG)pProc + 0xf4);
		if (!NT_SUCCESS(Status) || !(*pulObjectTable))
			continue;
		ulPID = *(ULONG*)((ULONG)pProc + 0xB4);
		// �ж�PID�Ƿ���ͬ
		if (ulPID != ulInputPID)
			continue;
		// ��ʼ�����ý��̵�ģ��
		pPeb = PsGetProcessPeb(pProc);
		// ��ǰ�߳��л����½��̶���
		KeAttachProcess(pProc);
		// ��ȡLDR��
		pLdrHeader = (LDR_DATA_TABLE_ENTRY*)pPeb->Ldr->InLoadOrderModuleList.Flink;
		pBegin = (LDR_DATA_TABLE_ENTRY*)pLdrHeader->InLoadOrderLinks.Blink;
		do 
		{
			RtlCopyMemory(pModuleTemp->FullDllName, pLdrHeader->FullDllName.Buffer, pLdrHeader->FullDllName.Length);	// ����
			pModuleTemp->dwSize = pLdrHeader->SizeOfImage;																// ��С
			pModuleTemp->dwStartAddress = (ULONG)pLdrHeader->DllBase;															// StartAddress
			pModuleTemp2 = ExAllocatePool(PagedPool, sizeof(MODULE));
			// memset(pModuleTemp2, 0, sizeof(MODULE));
			RtlZeroMemory(pModuleTemp2, sizeof(MODULE));
			pModuleTemp->Next = pModuleTemp2;
			pModuleTemp = pModuleTemp2;
			pLdrHeader = (LDR_DATA_TABLE_ENTRY*)pLdrHeader->InLoadOrderLinks.Flink;
			ulCount++;
		} while (pBegin != pLdrHeader);
		// ����ҿ�����
		KeDetachProcess();
		// �ݼ�һ�����ô���
		ObDereferenceObject(pProc);
		break;
	}
	ULONG OutPutSize = ulCount * sizeof(MODULE);
	// �����Ҫ�Ĵ�С�ȴ������Ĵ�С���򷵻���Ҫ�Ĵ�С
	PMODULE OutModule = pOutPut;
	if (OutPutSize <= ulBuffSize)
	{
		pModuleTemp = pModuleHead;
		for (ULONG i = 0; i < ulCount; ++i)
		{
			OutModule[i].dwSize = pModuleTemp->dwSize;									// Size
			OutModule[i].dwStartAddress = pModuleTemp->dwStartAddress;					// StartAddress
			RtlCopyMemory(OutModule[i].FullDllName, pModuleTemp->FullDllName, 256 * 2);	// FullDllName
			pModuleTemp = pModuleTemp->Next;
		}
	}
	// ����ռ�
	pModuleTemp = pModuleHead;
	for (ULONG i = 0; i <= ulCount; ++i)
	{
		ExFreePool(pModuleTemp);
		pModuleTemp = pModuleTemp->Next;
	}
	return OutPutSize;
}

//*****************************************************************************************
// ��������: HideProcess
// ����˵��: ���ؽ��� // ����ȥ��ActiveProcessLinks�еĽڵ�
// ��    ��: lracker
// ʱ    ��: 2019/12/25
// ��    ��: PVOID
// ��    ��: ULONG
// ��    ��: PVOID
// �� �� ֵ: ULONG
//*****************************************************************************************
VOID HideProcess(PVOID pPID)
{
	pPID;
	ULONG ulInputPID;
	ULONG EnumPID;
	PEPROCESS SystemEProcess;
	PLIST_ENTRY ulStartAddr;
	PLIST_ENTRY ulTemp;
	ulInputPID = *(ULONG*)pPID;
	SystemEProcess = PsGetCurrentProcess();
	ulStartAddr = (PLIST_ENTRY)((ULONG)SystemEProcess + 0xB8);
	ulTemp = ulStartAddr;
	do 
	{
		EnumPID = *(ULONG*)((ULONG)ulTemp - 0x4);
		// ����ҵ�������������
		if (EnumPID == ulInputPID)
		{
			ulTemp->Blink->Flink = ulTemp->Flink;
			ulTemp->Flink->Blink = ulTemp->Blink;
			ulTemp->Flink = NULL;
			ulTemp->Blink = NULL;
			break;
		}
		ulTemp = ulTemp->Flink;
	} while (ulTemp!=ulStartAddr);
}

//*****************************************************************************************
// ��������: TerminateProcess
// ����˵��: ��������
// ��    ��: lracker
// ʱ    ��: 2019/12/25
// ��    ��: PVOID
// �� �� ֵ: VOID
//*****************************************************************************************
VOID TerminateProcess(PVOID pPID)
{
	pPID;
	ULONG ulInputPID;
	HANDLE hProcess;
	ulInputPID = *(ULONG*)pPID;
	OBJECT_ATTRIBUTES ObjectAttributes;
	InitializeObjectAttributes(&ObjectAttributes, 0, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, 0, 0);
	CLIENT_ID ClientId;
	ClientId.UniqueProcess = (HANDLE)ulInputPID;
	ClientId.UniqueThread = 0;
	ZwOpenProcess(&hProcess, PROCESS_ALL_ACCESS, &ObjectAttributes, &ClientId);
	ZwTerminateProcess(hProcess, 0);
}

//*****************************************************************************************
// ��������: EnumFIle
// ����˵��: ����Ŀ¼�������ļ�
// ��    ��: lracker
// ʱ    ��: 2019/12/25
// ��    ��: PVOID
// ��    ��: ULONG
// ��    ��: PVOID
// �� �� ֵ: ULONG
//*****************************************************************************************
ULONG EnumFile(PVOID pOutPut, ULONG ulBuffSize, PVOID DirectoryName)
{
	ULONG ulRet;
	pOutPut;
	ulBuffSize;
	DirectoryName;
	UNICODE_STRING FullName;
	FullName.Buffer = (PWSTR)ExAllocatePool(PagedPool, 1024);
	FullName.MaximumLength = 1024;
	ULONG ulCount;
	ulCount = 0;
	UNICODE_STRING Path;
	RtlInitUnicodeString(&Path, DirectoryName);
	HANDLE hFile = NULL;
	SIZE_T nFileInfoSize = sizeof(FILE_BOTH_DIR_INFORMATION) + 270 * sizeof(WCHAR);
	SIZE_T nSize = nFileInfoSize * 0x256;
	char strFileName[0x256] = { 0 };
	PFILE_BOTH_DIR_INFORMATION pFileTemp = NULL;
	PFILE_BOTH_DIR_INFORMATION pFileList = NULL;
	pFileList = (PFILE_BOTH_DIR_INFORMATION)ExAllocatePool(PagedPool, nSize);
	pFileTemp = (PFILE_BOTH_DIR_INFORMATION)ExAllocatePool(PagedPool, nFileInfoSize);
	// ��·����װ�����ӷ������������ļ�
	RtlZeroMemory(pFileList, nSize);
	RtlZeroMemory(pFileTemp, nFileInfoSize);
	hFile = KernelCreateFile(&Path, TRUE);
	PENUMFILES pFileHead;
	PENUMFILES FileTemp;
	pFileHead = (PENUMFILES)ExAllocatePool(PagedPool, sizeof(ENUMFILES));
	RtlZeroMemory(pFileHead, sizeof(ENUMFILES));
	FileTemp = pFileHead;
	if (KernelFindFirstFile(hFile, nSize, pFileList, nFileInfoSize, pFileTemp))
	{
		LONG Loc = 0;
		do
		{
			RtlZeroMemory(strFileName, 0x256);
			RtlCopyMemory(strFileName, pFileTemp->FileName, pFileTemp->FileNameLength);
			if (strcmp(strFileName, "..") == 0 || strcmp(strFileName, ".") == 0)
				continue;
			++ulCount;
			if (pFileTemp->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				DbgPrint("[Ŀ¼]%S\n", strFileName);
				RtlCopyMemory(FileTemp->FileName, strFileName, 512);
				FileTemp->FileOrDirectory = 0;
			}
			else
			{ 
				DbgPrint("[�ļ�]%S\n", strFileName);
				RtlCopyMemory(FileTemp->FileName, strFileName, 512);	// �ļ���
				FileTemp->CreateTime = pFileTemp->CreationTime;			// ����ʱ��
				FileTemp->ChangeTime = pFileTemp->LastWriteTime;			// �޸�ʱ��
				UNICODE_STRING FileName;
				RtlInitUnicodeString(&FileName, (PCWSTR)strFileName);
				FullName.Length = 0;
				RtlZeroMemory(FullName.Buffer, 1024);
				RtlAppendUnicodeStringToString(&FullName, &Path);
				RtlAppendUnicodeStringToString(&FullName, &FileName);
				FileTemp->Size = GetFileSize(&FullName);					// �ļ�ȫ��
				FileTemp->FileOrDirectory = 1;
			}
			PENUMFILES FileTemp2;
			FileTemp2 = (PENUMFILES)ExAllocatePool(PagedPool, sizeof(ENUMFILES));
			FileTemp2->Next = NULL;
			FileTemp->Next = FileTemp2;
			FileTemp = FileTemp2;
		} while (KernelFindNextFile(pFileList, pFileTemp, &Loc));
	}
	ExFreePool(FullName.Buffer);
	ulRet = ZwClose(hFile);
	ULONG OutPutSize = ulCount * sizeof(ENUMFILES);
	// �����Ҫ�Ĵ�С�ȴ������Ĵ�С���򷵻���Ҫ�Ĵ�С
	PENUMFILES OutFiles = pOutPut;
	if (OutPutSize <= ulBuffSize)
	{
		FileTemp = pFileHead;
		for (ULONG i = 0; i < ulCount; ++i)
		{
			OutFiles[i].FileOrDirectory = FileTemp->FileOrDirectory;				// flags
			RtlCopyMemory(OutFiles[i].FileName, FileTemp->FileName, 256 * 2);		// �ļ���
			OutFiles[i].CreateTime = FileTemp->CreateTime;							// ����ʱ��
			OutFiles[i].ChangeTime = FileTemp->ChangeTime;							// �޸�ʱ��
			OutFiles[i].Size = FileTemp->Size;										// �ļ���С
			FileTemp = FileTemp->Next;
		}
	}
	// ����ռ�
	FileTemp = pFileHead;
	for (ULONG i = 0; i <= ulCount; ++i)
	{
		ExFreePool(FileTemp);
		FileTemp = FileTemp->Next;
	}
	return OutPutSize;
}

//*****************************************************************************************
// ��������: KernelFindFirstFile
// ����˵��: ��һ�β���
// ��    ��: lracker
// ʱ    ��: 2019/12/25
// ��    ��: HANDLE
// ��    ��: ULONG
// ��    ��: PFILE_BOTH_DIR_INFORMATION
// ��    ��: ULONG
// ��    ��: PFILE_BOTH_DIR_INFORMATION
// �� �� ֵ: BOOLEAN
//*****************************************************************************************
BOOLEAN KernelFindFirstFile(HANDLE hFile, ULONG ulLen, PFILE_BOTH_DIR_INFORMATION pDir, ULONG ulFirstLen, PFILE_BOTH_DIR_INFORMATION pFirstDir)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	IO_STATUS_BLOCK StatusBlock = { 0 };
	// ��ȡ��һ���ļ���Ϣ�����Ƿ�ɹ�
	Status = ZwQueryDirectoryFile(hFile, NULL, NULL, NULL, &StatusBlock, pFirstDir, ulFirstLen, FileBothDirectoryInformation, TRUE, NULL, FALSE);
	// ���ɹ������ȡ�ļ��б�
	if (NT_SUCCESS(Status) == FALSE)
		return FALSE;
	Status = ZwQueryDirectoryFile(hFile, NULL, NULL, NULL, &StatusBlock, pDir, ulLen, FileBothDirectoryInformation, FALSE, NULL, FALSE);
	return NT_SUCCESS(Status);
}


//*****************************************************************************************
// ��������: KernelFindNextFile
// ����˵��: ��ȡ��һ���ļ�
// ��    ��: lracker
// ʱ    ��: 2019/12/20
// ��    ��: pDirList, pDirInfo, loc
// �� �� ֵ: BOOLEAN
//*****************************************************************************************
BOOLEAN KernelFindNextFile(PFILE_BOTH_DIR_INFORMATION pDirList, PFILE_BOTH_DIR_INFORMATION pDirInfo, LONG* Loc)
{
	// �������һ����ƶ�ָ��ָ����һ��
	PFILE_BOTH_DIR_INFORMATION pDir = (PFILE_BOTH_DIR_INFORMATION)((PCHAR)pDirList + *Loc);
	LONG StructLength = 0;
	if (pDir->FileName[0] != 0)
	{
		StructLength = sizeof(FILE_BOTH_DIR_INFORMATION);
		memcpy(pDirInfo, pDir, StructLength + pDir->FileNameLength);
		*Loc = *Loc + pDir->NextEntryOffset;
		if (pDir->NextEntryOffset == 0)
			// ���û����һ����Ա����ָ���
			*Loc = *Loc + StructLength + pDir->FileNameLength;
		return TRUE;
	}
	return FALSE;
}

//*****************************************************************************************
// ��������: KernelCreateFile
// ����˵��: �����ļ�����
// ��    ��: lracker
// ʱ    ��: 2019/12/20
// ��    ��: pStrFile, bIsDIr
// �� �� ֵ: HANDLE
//*****************************************************************************************
HANDLE KernelCreateFile(PUNICODE_STRING pStrFile, BOOLEAN bIsDIr)
{
	HANDLE hFile = NULL;
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	IO_STATUS_BLOCK StatusBlock = { 0 };
	ULONG ulShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	ULONG ulCreateOpt = FILE_SYNCHRONOUS_IO_NONALERT;
	// ��ʼ��OBJECT_ATTRIBUTES������
	OBJECT_ATTRIBUTES objAttributes = { 0 };
	ULONG ulAttributes = OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE;
	InitializeObjectAttributes(&objAttributes, pStrFile, ulAttributes, NULL, NULL);
	// �����ļ�����
	ulCreateOpt |= bIsDIr ? FILE_DIRECTORY_FILE : FILE_NON_DIRECTORY_FILE;
	ntStatus = ZwCreateFile(&hFile, FILE_LIST_DIRECTORY | SYNCHRONIZE | FILE_ANY_ACCESS, &objAttributes, &StatusBlock, 0, FILE_ATTRIBUTE_NORMAL, ulShareAccess, FILE_OPEN_IF, ulCreateOpt, NULL, 0);
	if (!NT_SUCCESS(ntStatus))
		return (HANDLE)-1;
	return hFile;
}

//*****************************************************************************************
// ��������: DeleteFiles
// ����˵��: ɾ���ļ�
// ��    ��: lracker
// ʱ    ��: 2019/12/26
// ��    ��: PVOID
// �� �� ֵ: VOID
//*****************************************************************************************
BOOLEAN DeleteFiles(PVOID pFileName)
{
	NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
	UNICODE_STRING StrFile;
	RtlInitUnicodeString(&StrFile, pFileName);
	OBJECT_ATTRIBUTES objAttribute = { 0 };
	ULONG ulAttributes = OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE;
	InitializeObjectAttributes(&objAttribute, &StrFile, ulAttributes, NULL, NULL);
	// ɾ��ָ���ļ�/�ļ���
	ntStatus = ZwDeleteFile(&objAttribute);
	return TRUE;
}

//*****************************************************************************************
// ��������: GetFileSize
// ����˵��: ��ȡ�ļ��Ĵ�С
// ��    ��: lracker
// ʱ    ��: 2019/12/26
// ��    ��: HANDLE
// �� �� ֵ: LONGLONG
//*****************************************************************************************
LONGLONG GetFileSize(PUNICODE_STRING pFileStr)
{
	ULONG ulRet;
	IO_STATUS_BLOCK iosb = { 0 };
	OBJECT_ATTRIBUTES objAttributes = { 0 };
	ULONG ulAttributes = OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE;
	InitializeObjectAttributes(&objAttributes, pFileStr, ulAttributes, NULL, NULL);
	ULONG ulShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	FILE_STANDARD_INFORMATION fsi;
	HANDLE hFile;
	ULONG ulCreateOpt = FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE;
	ZwOpenFile(&hFile, FILE_READ_ATTRIBUTES, &objAttributes, &iosb, ulShareAccess, ulCreateOpt);
	ZwQueryInformationFile(hFile, &iosb, &fsi, sizeof(FILE_STANDARD_INFORMATION), FileStandardInformation);
	if (!hFile)
		return 0;
	__try
	{ 
		ulRet = ZwClose(hFile);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) 
	{
		DbgPrint("ERROR\n");
	}
	return fsi.EndOfFile.QuadPart;
}

//*****************************************************************************************
// ��������: EnumIDT
// ����˵��: ����IDT��
// ��    ��: lracker
// ʱ    ��: 2019/12/26
// ��    ��: PVOID
// ��    ��: ULONG
// �� �� ֵ: ULONG
//*****************************************************************************************
BOOLEAN EnumIDT(PVOID pOutPut, ULONG ulBuffSize)
{
	IDT_INFO SIDT1 = { 0,0,0 };
	PIDT_ENTRY pIDTEntry = NULL;
	PIDT_ENTRY pOut = (PIDT_ENTRY)pOutPut;
	ULONG uAddr = 0;
	// ��ȡIDT���ַ
	__asm sidt SIDT1;
	// ��ȡIDT�������ַ
	pIDTEntry = (PIDT_ENTRY)(SIDT1.uLowIdtBase + (SIDT1.uHighIdtBase << 16));
	// ��ȡIDT��Ϣ
	for (ULONG i = 0; i < 0x100; ++i)
	{
		pOut[i].uOffsetLow = pIDTEntry[i].uOffsetLow;		// �͵�ַ
		pOut[i].uOffsetHigh = pIDTEntry[i].uOffsetHigh;		// �ߵ�ַ
		pOut[i].uSelector = pIDTEntry[i].uSelector;			// ��ѡ����
		pOut[i].uType = pIDTEntry[i].uType;					// ����
		pOut[i].uDpl = pIDTEntry[i].uDpl;					// ��Ȩ�ȼ�
	}
	return TRUE;
}

//*****************************************************************************************
// ��������: EnumGDT
// ����˵��: ����GDT��
// ��    ��: lracker
// ʱ    ��: 2019/12/26
// ��    ��: PVOID
// ��    ��: ULONG
// �� �� ֵ: BOOLEAN
//*****************************************************************************************
ULONG EnumGDT(PVOID pOutPut, ULONG ulBuffSize)
{
	GDT_INFO SGDT1 = { 0,0,0 };
	PGDT_ENTRY pGDTEntry = NULL;
	PGDT_ENTRY pOut = (PGDT_ENTRY)pOutPut;
	ULONG uAddr = 0;
	// ��ȡGDT���ַ
	__asm sgdt SGDT1;
	// ��ȡGDT�������ַ
	ULONG ulNum = SGDT1.uGdtlimit / 8;
	pGDTEntry = (PGDT_ENTRY)(SGDT1.uLowGdtBase + (SGDT1.uHighGdtBase << 16));
	ULONG ulRetNum = 0;
	for (ULONG i = 0; i < ulNum; ++i)
	{
		// ����β����ڵĻ����򲻱�������
		if (pGDTEntry[i].P == 0)
			continue;
		ulRetNum++;
	}
	if (ulRetNum * sizeof(GDT_ENTRY) > ulBuffSize)
		return ulRetNum * sizeof(GDT_ENTRY);
	for (ULONG i = 0; i < ulRetNum; ++i)
	{
		// ����β����ڵĻ����򲻱�������
		if(pGDTEntry[i].P == 0)
			continue;
		RtlCopyMemory(&pOut[i], &pGDTEntry[i], sizeof(GDT_ENTRY));
	}
	return ulRetNum * sizeof(GDT_ENTRY);
}

//*****************************************************************************************
// ��������: EnumSSDT
// ����˵��: ����SSDT��
// ��    ��: lracker
// ʱ    ��: 2019/12/26
// ��    ��: PVOID
// ��    ��: ULONG
// �� �� ֵ: ULONG
//*****************************************************************************************
ULONG EnumSSDT(PVOID pOutPut, ULONG ulBuffSize)
{
	PETHREAD pThread = PsGetCurrentThread();
	PSystemServiceDescriptorTable pSystemServiceDescriptor = (PSystemServiceDescriptorTable)*(ULONG*)((ULONG)pThread + 0xbc);
	LONG ulNum = pSystemServiceDescriptor->NumberOfService;
	if (ulBuffSize < ulNum * sizeof(SSDT))
		return ulNum * sizeof(SSDT);
	PSSDT pSSDT = (PSSDT)pOutPut;
	PLONG Addr = pSystemServiceDescriptor->ServiceTableBase;
	for (int i = 0; i < ulNum; ++i)
	{
		pSSDT[i].Address = Addr[i];
		pSSDT[i].SysCallIndex = i;
	}
	return ulNum * sizeof(SSDT);
}

//*****************************************************************************************
// ��������: EnumRegister
// ����˵��: ����ע���
// ��    ��: lracker
// ʱ    ��: 2019/12/27
// ��    ��: PVOID
// ��    ��: ULONG
// ��    ��: PVOID
// �� �� ֵ: ULONG
//*****************************************************************************************
ULONG EnumRegister(PVOID pOutPut, ULONG ulBuffSize, PVOID pRegisterName)
{
	ULONG ulCount = 0;
	ULONG ulSize, i;
	NTSTATUS ntStatus;
	PKEY_VALUE_PARTIAL_INFORMATION pvpi;
	PKEY_FULL_INFORMATION pfi;
	PKEY_FULL_INFORMATION pfi2;
	OBJECT_ATTRIBUTES objectAttributes;
	HANDLE hRegister;
	UNICODE_STRING RegUnicodeString;
	PREGISTER pReg = (PREGISTER)pOutPut;
	// ��ʼ��UNICODE_STRING�ַ���
	RtlInitUnicodeString(&RegUnicodeString, pRegisterName);
	InitializeObjectAttributes(&objectAttributes, &RegUnicodeString, OBJ_CASE_INSENSITIVE, NULL, NULL);
	// ��ע���
	ntStatus = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &objectAttributes);
	if (NT_SUCCESS(ntStatus))
		DbgPrint("Open Register Successfully\n");
	// ��ѯVALUE�Ĵ�С
	ZwQueryKey(hRegister, KeyFullInformation, NULL, 0, &ulSize);
	pfi = (PKEY_FULL_INFORMATION)ExAllocatePool(PagedPool, ulSize);
	ZwQueryKey(hRegister, KeyFullInformation, pfi, ulSize, &ulSize);
	ulCount = pfi->SubKeys + pfi->Values;
	// �жϴ������Ĵ�С�Ƿ���ȷ
	if (ulCount * sizeof(REGISTER) > ulBuffSize)
		return ulCount * sizeof(REGISTER);
	// �������������
	ULONG ulIndex = 0;
	for (i = 0; i < pfi->SubKeys; ++i)
	{
		pReg[ulIndex].Type = 0;
		ULONG ulSize = 0;
		ZwEnumerateKey(hRegister, i, KeyBasicInformation, NULL, 0, &ulSize);
		PKEY_BASIC_INFORMATION pbi = (PKEY_BASIC_INFORMATION)ExAllocatePool(PagedPool, ulSize);
		ZwEnumerateKey(hRegister, i, KeyBasicInformation, pbi, ulSize, &ulSize);
		RtlCopyMemory(&pReg[ulIndex].KeyName, pbi->Name, pbi->NameLength);
		ExFreePool(pbi);
		ulIndex++;
	}
	for (i = 0; i < pfi->Values; ++i)
	{
		pReg[ulIndex].Type = 1;
		PKEY_VALUE_FULL_INFORMATION pvbi;
		//��ѯ����VALUE�Ĵ�С
		ZwEnumerateValueKey(hRegister, i, KeyValueFullInformation, NULL, 0, &ulSize);
		pvbi = (PKEY_VALUE_FULL_INFORMATION)ExAllocatePool(PagedPool, ulSize);
		//��ѯ����VALUE������
		ZwEnumerateValueKey(hRegister, i, KeyValueFullInformation, pvbi, ulSize, &ulSize);
		// ��ȡ��ֵ������
		RtlCopyMemory(&pReg[ulIndex].ValueName, pvbi->Name, pvbi->NameLength);
		pReg[ulIndex].ValueType = pvbi->Type;
		RtlCopyMemory(pReg[ulIndex].Value, (ULONG)pvbi + pvbi->DataOffset, pvbi->DataLength);
		ExFreePool(pvbi);
		++ulIndex;
	}
	ExFreePool(pfi);
	ZwClose(hRegister);
	return ulCount * sizeof(REGISTER);
}

//*****************************************************************************************
// ��������: NewReg
// ����˵��: ��������
// ��    ��: lracker
// ʱ    ��: 2019/12/28
// ��    ��: PVOID
// �� �� ֵ: ULONG
//*****************************************************************************************
ULONG NewReg(PVOID pRegisterName)
{
	OBJECT_ATTRIBUTES objectAttributes;
	UNICODE_STRING usKeyName;
	NTSTATUS ntStatus;
	HANDLE hRegister;
	RtlInitUnicodeString(&usKeyName, pRegisterName);
	// ��ʼ��������Ϣ(ע����·��)
	InitializeObjectAttributes(&objectAttributes,
		&usKeyName,
		OBJ_CASE_INSENSITIVE,//�Դ�Сд����
		NULL,
		NULL);
	ntStatus = ZwCreateKey(&hRegister,/*�����ע�����*/
		KEY_ALL_ACCESS,
		&objectAttributes,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		NULL);
	if (NT_SUCCESS(ntStatus)) {
		ZwClose(hRegister);
	}
	return ntStatus;
}

//*****************************************************************************************
// ��������: DeleteReg
// ����˵��: ɾ��ע���
// ��    ��: lracker
// ʱ    ��: 2019/12/28
// ��    ��: PVOID
// �� �� ֵ: ULONG
//*****************************************************************************************
ULONG DeleteReg(PVOID pRegisterName)
{
	OBJECT_ATTRIBUTES objectAttributes;
	UNICODE_STRING usKeyName;
	NTSTATUS ntStatus;
	HANDLE hRegister;
	RtlInitUnicodeString(&usKeyName, pRegisterName);
	// ��ʼ��ע����·��
	InitializeObjectAttributes(&objectAttributes,
		&usKeyName,
		OBJ_CASE_INSENSITIVE,//�Դ�Сд����
		NULL,
		NULL);
	// ��ע���,��ȡע������·��
	ntStatus = ZwOpenKey(&hRegister, KEY_ALL_ACCESS, &objectAttributes);
	if (NT_SUCCESS(ntStatus)) {
		// ɾ��ע����
		ntStatus = ZwDeleteKey(hRegister);
		ZwClose(hRegister);
		DbgPrint("ZwDeleteKey success!\n");
	}
	return ntStatus;
}


