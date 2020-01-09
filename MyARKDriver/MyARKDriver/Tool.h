#pragma once
#include <ntifs.h>

// ��������
ULONG EnumDriver(PVOID pOutPut, ULONG ulBuffSize, PDRIVER_OBJECT pDriver);

// ��������
UINT8 HideDriver(UNICODE_STRING DriverName, PDRIVER_OBJECT pDriver);

// ��������
ULONG EnumProcess(PVOID pOutPut, ULONG ulBuffSize);

// �����߳�
ULONG EnumThread(PVOID pOutPut, ULONG ulBuffSize, PVOID pPID);

// ����ģ��
ULONG EnumModule(PVOID pOutPut, ULONG ulBuffSize, PVOID pPID);

// ���ؽ���
VOID HideProcess(PVOID pPID);

// ��������
VOID TerminateProcess(PVOID pPID);

// ����Ŀ¼�����е��ļ�
ULONG EnumFile(PVOID pOutPut, ULONG ulBuffSize, PVOID DirectoryName);

// FindFirstFile
BOOLEAN KernelFindFirstFile(HANDLE hFile, ULONG ulLen, PFILE_BOTH_DIR_INFORMATION pDIr, ULONG ulFirstLen, PFILE_BOTH_DIR_INFORMATION pFirstDir);

// FindNextFile
BOOLEAN KernelFindNextFile(PFILE_BOTH_DIR_INFORMATION pDirList, PFILE_BOTH_DIR_INFORMATION pDirInfo, LONG* Loc);

// ���ļ�
HANDLE KernelCreateFile(PUNICODE_STRING pStrFile, BOOLEAN bIsDIr);

// ɾ���ļ�
BOOLEAN DeleteFiles(PVOID FileName);

// ��ȡ�ļ���С
LONGLONG GetFileSize(PUNICODE_STRING pFileStr);

// ����IDT��
BOOLEAN EnumIDT(PVOID pOutPut, ULONG ulBuffSize);

// ����GDT��
ULONG EnumGDT(PVOID pOutPut, ULONG ulBuffSize);

// ����SSDT��
ULONG EnumSSDT(PVOID pOutPut, ULONG ulBuffSize);

// ����ע���
ULONG EnumRegister(PVOID pOutPut, ULONG ulBuffSize, PVOID RegisterName);

// ��������
ULONG NewReg(PVOID pRegisterName);

// ɾ������
ULONG DeleteReg(PVOID pRegisterName);