#include "Tool.h"
#include <ntimage.h>
#include "KernelFunction.h"
#define MYCTLCODE(code)CTL_CODE(FILE_DEVICE_UNKNOWN,0x800+(code),METHOD_OUT_DIRECT ,FILE_ANY_ACCESS)
typedef enum _MyCtlCode
{
	ENUMDRIVER = MYCTLCODE(0),
	HIDEDRIVER = MYCTLCODE(1),
	ENUMPROCESS = MYCTLCODE(2),
	ENUMTHREAD = MYCTLCODE(3),
	ENUMMODULE = MYCTLCODE(4),
	HIDEPROCESS = MYCTLCODE(5),
	TERMINATEPROCESS = MYCTLCODE(6),
	ENUMFILE = MYCTLCODE(7),
	DELETEFILE = MYCTLCODE(8),
	ENUMIDT = MYCTLCODE(9),
	ENUMGDT = MYCTLCODE(10),
	ENUMSSDT = MYCTLCODE(11),
	GETPID = MYCTLCODE(12),
	ENUMREGISTER = MYCTLCODE(13),
	NEWREG = MYCTLCODE(14),
	DELETEREG = MYCTLCODE(15),
}MyCtlCode;

#define NAME_DEVICE L"\\Device\\MyDevice"
#define NAME_SYMBOL L"\\DosDevices\\Device_001"
PDRIVER_OBJECT g_pDriver;

NTSTATUS Unload(PDRIVER_OBJECT pDriver);
NTSTATUS Create(PDEVICE_OBJECT pDevice, PIRP pIrp);
NTSTATUS Close(PDEVICE_OBJECT pDevice, PIRP pIrp);
NTSTATUS OnDeviceIoControl(PDEVICE_OBJECT pDevice, PIRP pIrp);
VOID InstallSystenerHook();
VOID UninstallSysenterHook();
VOID MyKiFastCallEntry();
ULONG_PTR g_OldKiFastCallEntry;
ULONG g_uPid;

////////////////////////////////////////////
/////////////  �ں�������غ���   /////////////
////////////////////////////////////////////

// ����SSDTȫ�ֱ���
NTSYSAPI SSDTEntry	KeServiceDescriptorTable;


static char*		g_pNewNtKernel;		// ���ں�
static ULONG		g_ntKernelSize;		// �ں˵�ӳ���С
static SSDTEntry* g_pNewSSDTEntry;	// ��ssdt����ڵ�ַ	
static ULONG		g_hookAddr;			// ��hookλ�õ��׵�ַ
static ULONG		g_hookAddr_next_ins;// ��hook��ָ�����һ��ָ����׵�ַ.


// ��ȡNT�ں�ģ��
// ����ȡ���Ļ����������ݱ��浽pBuff��.
NTSTATUS loadNtKernelModule(UNICODE_STRING* ntkernelPath, char** pBuff);

// �޸��ض�λ.
void fixRelocation(char* pDosHdr, ULONG curNtKernelBase);

// ���SSDT��
// char* pDos - �¼��ص��ں˶ѿռ��׵�ַ
// char* pCurKernelBase - ��ǰ����ʹ�õ��ں˵ļ��ػ�ַ
void initSSDT(char* pDos, char* pCurKernelBase);

// ��װHOOK
void installHook();

// ж��HOOK
void uninstallHook();

// inline hook KiFastCallEntry�ĺ���
void myKiFastEntryHook();

	
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING pPath)
{
	NTSTATUS Status = STATUS_SUCCESS;
	g_pDriver = pDriver;
	pPath;

	// �����ں�����
	// 1. �ҵ��ں��ļ�·��
	// 1.1 ͨ�������ں�����ķ�ʽ���ҵ��ں���ģ��
	LDR_DATA_TABLE_ENTRY* pLdr = ((LDR_DATA_TABLE_ENTRY*)pDriver->DriverSection);
	// 1.2 �ں���ģ���������еĵ�2��.
	for (int i = 0; i < 2; ++i)
		pLdr = (LDR_DATA_TABLE_ENTRY*)pLdr->InLoadOrderLinks.Flink;

	g_ntKernelSize = pLdr->SizeOfImage;

	// 1.3 ���浱ǰ���ػ�ַ
	char* pCurKernelBase = (char*)pLdr->DllBase;

	KdPrint(("Old base=%p name=%wZ\n", pCurKernelBase, &pLdr->FullDllName));

	// 2. ��ȡntģ����ļ����ݵ��ѿռ�.
	Status = loadNtKernelModule(&pLdr->FullDllName, &g_pNewNtKernel);
	if (STATUS_SUCCESS != Status)
	{
		return Status;
	}

	// 3. �޸���ntģ����ض�λ.
	fixRelocation(g_pNewNtKernel, (ULONG)pCurKernelBase);

	// 4. ʹ�õ�ǰ����ʹ�õ��ں˵����������
	//    ���ں˵�SSDT��.
	initSSDT(g_pNewNtKernel, pCurKernelBase);

	// 5. HOOK KiFastCallEntry,ʹ���ú������ں˵�·��
	installHook();
	KdPrint(("New base=%p name=%wZ\n", g_pNewNtKernel, &pLdr->FullDllName));
	// �ں����ؽ���


	// ����һ���豸����
	UNICODE_STRING DevName;
	RtlInitUnicodeString(&DevName, NAME_DEVICE);
	PDEVICE_OBJECT pDev = NULL;
	Status = IoCreateDevice(pDriver, 0, &DevName, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDev);
	if (Status != STATUS_SUCCESS)
	{
		DbgPrint("�����豸ʧ��,������:%08X\n", Status);
		return Status;
	}
	pDev->Flags |= DO_DIRECT_IO;
	UNICODE_STRING DosSymName;
	RtlInitUnicodeString(&DosSymName, NAME_SYMBOL);
	IoCreateSymbolicLink(&DosSymName, &DevName);
	// ��ж�غ���
	pDriver->DriverUnload = Unload;
	// ����ǲ����
	pDriver->MajorFunction[IRP_MJ_CREATE] = Create;
	pDriver->MajorFunction[IRP_MJ_CLOSE] = Close;
	pDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = OnDeviceIoControl;
	// ��װHOOK
	InstallSystenerHook();
	return Status;
}

//*****************************************************************************************
// ��������: Unload
// ����˵��: ж�غ���
// ��    ��: lracker
// ʱ    ��: 2019/12/23
// ��    ��: pDriver
// �� �� ֵ: NTSTATUS
//*****************************************************************************************
NTSTATUS Unload(PDRIVER_OBJECT pDriver)
{
	// ж��HOOK
	UninstallSysenterHook();
	// ɾ����������
	UNICODE_STRING DosSymName;
	RtlInitUnicodeString(&DosSymName, NAME_SYMBOL);
	IoDeleteSymbolicLink(&DosSymName);
	// ж���豸����
	IoDeleteDevice(pDriver->DeviceObject);
	UNREFERENCED_PARAMETER(pDriver);
	uninstallHook();
	return STATUS_SUCCESS;
}

//*****************************************************************************************
// ��������: Create
// ����˵��: ��������
// ��    ��: lracker
// ʱ    ��: 2019/12/23
// ��    ��: pDevice, pIrp
// �� �� ֵ: NTSTATUS
//*****************************************************************************************
NTSTATUS Create(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	pDevice;
	DbgPrint("������������");
	// ����IRP���״̬
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//*****************************************************************************************
// ��������: Close
// ����˵��: �رպ���
// ��    ��: lracker
// ʱ    ��: 2019/12/23
// ��    ��: pDevice, pIrp
// �� �� ֵ: NTSTATUS
//*****************************************************************************************
NTSTATUS Close(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	pDevice;
	DbgPrint("�������ر���");
	// ����IRP���״̬
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

//*****************************************************************************************
// ��������: OnDeviceIoControl
// ����˵��: ���ƺ��������ڷַ�IRP��Ϣ
// ��    ��: lracker
// ʱ    ��: 2019/12/23
// ��    ��: pDevice, pIrp
// �� �� ֵ: NTSTATUS
//*****************************************************************************************
NTSTATUS OnDeviceIoControl(PDEVICE_OBJECT pDevice, PIRP pIrp)
{
	pDevice;
	NTSTATUS ntStatus = STATUS_SUCCESS;
	ULONG ulComplateSize = 0;
	// ����IRP���״̬
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	// ��ȡ����IOջ
	IO_STACK_LOCATION* pIoStack = IoGetCurrentIrpStackLocation(pIrp);
	ULONG uCtrlCode = pIoStack->Parameters.DeviceIoControl.IoControlCode;
	PVOID pInputBuff = pIrp->AssociatedIrp.SystemBuffer;
	PVOID pOutBuff = NULL;
	if (pIrp->MdlAddress && METHOD_FROM_CTL_CODE(uCtrlCode) & METHOD_OUT_DIRECT)
		pOutBuff = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, 0);
	ULONG ulBuffSize = pIoStack->Parameters.DeviceIoControl.OutputBufferLength;
	switch (uCtrlCode)
	{
		// ��������
		case ENUMDRIVER:
		{
			DbgPrint("��������\n");
			ulComplateSize = EnumDriver(pOutBuff, ulBuffSize, g_pDriver);
			break;
		}
		// ��������
		case HIDEDRIVER:
		{
			PWCHAR Input = (PWCHAR)pInputBuff;
			UNICODE_STRING DriverName = { 0 };
			RtlInitUnicodeString(&DriverName, Input);
			DbgPrint("��������: %S\n", DriverName.Buffer);
			UINT8 Ret;
			Ret = HideDriver(DriverName, g_pDriver);
			break;
		}
		// ��������
		case ENUMPROCESS:
		{
			DbgPrint("��������\n");
			ulComplateSize = EnumProcess(pOutBuff, ulBuffSize);
			break;
		}
		// �����߳�
		case ENUMTHREAD:
		{
			DbgPrint("�����߳�\n");
			ulComplateSize = EnumThread(pOutBuff, ulBuffSize, pInputBuff);
			break;
		}
		// ��������ģ��
		case ENUMMODULE:
		{
			DbgPrint("��������ģ��\n");
			ulComplateSize = EnumModule(pOutBuff, ulBuffSize, pInputBuff);
			break;
		}
		// ���ؽ���
		case HIDEPROCESS:
		{
			DbgPrint("���ؽ���\n");
			HideProcess(pInputBuff);
			break;
		}
		// ��������
		case TERMINATEPROCESS:
		{
			DbgPrint("��������\n");
			TerminateProcess(pInputBuff);
			break;
		}
		// ������Ŀ¼�µ������ļ�
		case ENUMFILE:
		{
			DbgPrint("�����ļ�\n");
			ulComplateSize = EnumFile(pOutBuff, ulBuffSize, pInputBuff);
			break;
		}
		// ɾ���ļ�
		case DELETEFILE:
		{
			DbgPrint("ɾ���ļ�\n");
			DeleteFiles(pInputBuff);
			break;
		}
		// ����IDT��
		case ENUMIDT:
		{
			DbgPrint("����IDT��\n");
			ulComplateSize = EnumIDT(pOutBuff, ulBuffSize);
			break;
		}
		// ����GDT
		case ENUMGDT:
		{
			DbgPrint("����GDT��\n");
			ulComplateSize = EnumGDT(pOutBuff, ulBuffSize);
			break;
		}
		// ����SSDT
		case ENUMSSDT:
		{
			DbgPrint("����SSDT\n");
			ulComplateSize = EnumSSDT(pOutBuff, ulBuffSize);
			break;
		}
		// �������PID
		case GETPID:
		{
			ULONG PID;
			PID = *(ULONG*)pInputBuff;
			DbgPrint("��ȡ��PID: %d", PID);
			// g_uPid = PID;
			g_uPid = 0;
			break;
		}
		// ����ע���
		case ENUMREGISTER:
		{
			DbgPrint("����ע���\n");
			ulComplateSize = EnumRegister(pOutBuff, ulBuffSize, pInputBuff);
			break;
		}
		// ��������
		case NEWREG:
		{
			DbgPrint("��������\n");
			NewReg(pInputBuff);
			break;
		}
		// ɾ������
		case DELETEREG:
		{
			DbgPrint("ɾ������\n");
			DeleteReg(pInputBuff);
			break;
		}
	}
	DbgPrint("�豸������\n");
	pIrp->IoStatus.Status = ntStatus;
	pIrp->IoStatus.Information = ulComplateSize;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return ntStatus;
}


VOID _declspec(naked) InstallSystenerHook()
{
	__asm 
	{
		push edx;
		push eax;
		push ecx;
		// ����ԭʼ����
		mov ecx, 0x176;					// SYSTENTER_EIP_MSR�Ĵ����ı��
		rdmsr;							// 
		mov[g_OldKiFastCallEntry], eax;	// ���ɵĵ�ַ���浽ȫ�ֱ�����
		// ���µĺ������ý�ȥ
		mov eax, MyKiFastCallEntry;
		xor edx, edx;
		wrmsr;							// ��edx:eax д��Ĵ����� 
		pop ecx;
		pop eax;
		pop edx;
		ret;
	}
}


VOID _declspec(naked) MyKiFastCallEntry()
{
	__asm
	{
		// �����ú�
		cmp eax, 0xBE;
		jne _DONE;		// ���úŲ���0xBE,ִ�е��Ĳ�

		push eax;	
		mov eax, [edx + 0x14];	// eax������PCLIENT_ID
		mov eax, [eax];			// eax������PID
		
		cmp eax, [g_uPid];
		pop eax;				// �ָ��Ĵ���
		jne _DONE;				// ����Ҫ�����Ľ��̾���ת

		mov[edx + 0xc], 0;		// ������Ȩ������Ϊ0
	_DONE:
		// ����ԭ����KiFastCallEntry
		jmp g_OldKiFastCallEntry;
	}
}


// ж��HOOK
VOID UninstallSysenterHook()
{
	__asm
	{
		push edx;
		push eax;
		push ecx;
		mov eax, [g_OldKiFastCallEntry];
		xor edx, edx;
		mov ecx, 0x176;
		wrmsr;
		pop ecx;
		pop eax; 
		pop edx;
	}
}


// �ر��ڴ�ҳд�뱣��
void _declspec(naked) disablePageWriteProtect()
{
	_asm
	{
		push eax;
		mov eax, cr0;
		and eax, ~0x10000;
		mov cr0, eax;
		pop eax;
		ret;
	}
}

// �����ڴ�ҳд�뱣��
void _declspec(naked) enablePageWriteProtect()
{
	_asm
	{
		push eax;
		mov eax, cr0;
		or eax, 0x10000;
		mov cr0, eax;
		pop eax;
		ret;
	}
}


// ����NT�ں�ģ��
// ����ȡ���Ļ����������ݱ��浽pBuff��.
NTSTATUS loadNtKernelModule(UNICODE_STRING* ntkernelPath, char** pOut)
{
	NTSTATUS status = STATUS_SUCCESS;
	// 2. ��ȡ�ļ��е��ں�ģ��
	// 2.1 ���ں�ģ����Ϊ�ļ�����.
	HANDLE hFile = NULL;
	char* pBuff = NULL;
	ULONG read = 0;
	char pKernelBuff[0x1000];

	status = createFile(ntkernelPath->Buffer,
		GENERIC_READ,
		FILE_SHARE_READ,
		FILE_OPEN_IF,
		FALSE,
		&hFile);
	if (status != STATUS_SUCCESS)
	{
		KdPrint(("���ļ�ʧ��\n"));
		goto _DONE;
	}

	// 2.2 ��PE�ļ�ͷ����ȡ���ڴ�
	status = readFile(hFile, 0, 0, 0x1000, pKernelBuff, &read);
	if (STATUS_SUCCESS != status)
	{
		KdPrint(("��ȡ�ļ�����ʧ��\n"));
		goto _DONE;
	}

	// 3. ����PE�ļ����ڴ�.
	// 3.1 �õ���չͷ,��ȡӳ���С. 
	IMAGE_DOS_HEADER* pDos = (IMAGE_DOS_HEADER*)pKernelBuff;
	IMAGE_NT_HEADERS* pnt = (IMAGE_NT_HEADERS*)((ULONG)pDos + pDos->e_lfanew);
	ULONG imgSize = pnt->OptionalHeader.SizeOfImage;

	// 3.2 �����ڴ��Ա���������ε�����.
	pBuff = ExAllocatePool(NonPagedPool, imgSize);
	if (pBuff == NULL)
	{
		KdPrint(("�ڴ�����ʧ��ʧ��\n"));
		status = STATUS_BUFFER_ALL_ZEROS;//��㷵�ظ�������
		goto _DONE;
	}

	// 3.2.1 ����ͷ�����ѿռ�
	RtlCopyMemory(pBuff,
		pKernelBuff,
		pnt->OptionalHeader.SizeOfHeaders);

	// 3.3 �õ�����ͷ, ������������ͷ�����ζ�ȡ���ڴ���.
	IMAGE_SECTION_HEADER* pScnHdr = IMAGE_FIRST_SECTION(pnt);
	ULONG scnCount = pnt->FileHeader.NumberOfSections;
	for (ULONG i = 0; i < scnCount; ++i)
	{
		//
		// 3.3.1 ��ȡ�ļ����ݵ��ѿռ�ָ��λ��.
		//
		status = readFile(hFile,
			pScnHdr[i].PointerToRawData,
			0,
			pScnHdr[i].SizeOfRawData,
			pScnHdr[i].VirtualAddress + pBuff,
			&read);
		if (status != STATUS_SUCCESS)
			goto _DONE;

	}


_DONE:
	ZwClose(hFile);
	//
	// �������ں˵ļ��ص��׵�ַ
	//
	*pOut = pBuff;

	if (status != STATUS_SUCCESS)
	{
		if (pBuff != NULL)
		{
			ExFreePool(pBuff);
			*pOut = pBuff = NULL;
		}
	}
	return status;
}


// �޸��ض�λ.
void fixRelocation(char* pDosHdr, ULONG curNtKernelBase)
{
	IMAGE_DOS_HEADER* pDos = (IMAGE_DOS_HEADER*)pDosHdr;
	IMAGE_NT_HEADERS* pNt = (IMAGE_NT_HEADERS*)((ULONG)pDos + pDos->e_lfanew);
	ULONG uRelRva = pNt->OptionalHeader.DataDirectory[5].VirtualAddress;
	IMAGE_BASE_RELOCATION* pRel =
		(IMAGE_BASE_RELOCATION*)(uRelRva + (ULONG)pDos);

	while (pRel->SizeOfBlock)
	{
		typedef struct
		{
			USHORT offset : 12;
			USHORT type : 4;
		}TypeOffset;

		ULONG count = (pRel->SizeOfBlock - 8) / 2;
		TypeOffset* pTypeOffset = (TypeOffset*)(pRel + 1);
		for (ULONG i = 0; i < count; ++i)
		{
			if (pTypeOffset[i].type != 3)
			{
				continue;
			}

			ULONG* pFixAddr = (ULONG*)(pTypeOffset[i].offset + pRel->VirtualAddress + (ULONG)pDos);
			//
			// ��ȥĬ�ϼ��ػ�ַ
			//
			*pFixAddr -= pNt->OptionalHeader.ImageBase;

			//
			// �����µļ��ػ�ַ(ʹ�õ��ǵ�ǰ�ں˵ļ��ػ�ַ,������
			// ��Ϊ�������ں�ʹ�õ�ǰ�ں˵�����(ȫ�ֱ���,δ��ʼ������������).)
			//
			*pFixAddr += (ULONG)curNtKernelBase;
		}

		pRel = (IMAGE_BASE_RELOCATION*)((ULONG)pRel + pRel->SizeOfBlock);
	}
}

// ���SSDT��
// char* pNewBase - �¼��ص��ں˶ѿռ��׵�ַ
// char* pCurKernelBase - ��ǰ����ʹ�õ��ں˵ļ��ػ�ַ
void initSSDT(char* pNewBase, char* pCurKernelBase)
{
	// 1. �ֱ��ȡ��ǰ�ں�,�¼��ص��ں˵�`KeServiceDescriptorTable`
	//    �ĵ�ַ
	SSDTEntry* pCurSSDTEnt = &KeServiceDescriptorTable;
	g_pNewSSDTEntry = (SSDTEntry*)
		((ULONG)pCurSSDTEnt - (ULONG)pCurKernelBase + (ULONG)pNewBase);

	// 2. ��ȡ�¼��ص��ں��������ű�ĵ�ַ:
	// 2.1 ���������ַ
	g_pNewSSDTEntry->ServiceTableBase = (ULONG*)
		((ULONG)pCurSSDTEnt->ServiceTableBase - (ULONG)pCurKernelBase + (ULONG)pNewBase);

	// 2.3 �����������ֽ������ַ
	g_pNewSSDTEntry->ParamTableBase = (ULONG*)
		((ULONG)pCurSSDTEnt->ParamTableBase - (ULONG)pCurKernelBase + (ULONG)pNewBase);

	// 2.2 ���������ô������ַ(��ʱ�������������.)
	if (pCurSSDTEnt->ServiceCounterTableBase)
	{
		g_pNewSSDTEntry->ServiceCounterTableBase = (ULONG*)
			((ULONG)pCurSSDTEnt->ServiceCounterTableBase - (ULONG)pCurKernelBase + (ULONG)pNewBase);
	}

	// 2.3 ������SSDT��ķ������
	g_pNewSSDTEntry->NumberOfServices = pCurSSDTEnt->NumberOfServices;

	//3. ���������ĵ�ַ��䵽��SSDT��(�ض�λʱ��ʵ�Ѿ��޸�����,
	//   ����,���޸��ض�λ��ʱ��,��ʹ�õ�ǰ�ں˵ļ��ػ�ַ��, �޸��ض�λ
	//   Ϊ֮��, ���ں˵�SSDT����ķ������ĵ�ַ���ǵ�ǰ�ں˵ĵ�ַ,
	//   ������Ҫ����Щ�������ĵ�ַ�Ļ����ں��еĺ�����ַ.)
	disablePageWriteProtect();
	for (ULONG i = 0; i < g_pNewSSDTEntry->NumberOfServices; ++i)
	{
		// ��ȥ��ǰ�ں˵ļ��ػ�ַ
		g_pNewSSDTEntry->ServiceTableBase[i] -= (ULONG)pCurKernelBase;
		// �������ں˵ļ��ػ�ַ.
		g_pNewSSDTEntry->ServiceTableBase[i] += (ULONG)pNewBase;
	}
	enablePageWriteProtect();
}

void installHook()
{
	g_hookAddr = 0;

	// 1. �ҵ�KiFastCallEntry�����׵�ַ
	ULONG uKiFastCallEntry = 0;
	_asm
	{
		;// KiFastCallEntry������ַ����
		;// ������ģ��Ĵ�����0x176�żĴ�����
		push ecx;
		push eax;
		push edx;
		mov ecx, 0x176; // ���ñ��
		rdmsr; ;// ��ȡ��edx:eax
		mov uKiFastCallEntry, eax;// ���浽����
		pop edx;
		pop eax;
		pop ecx;
	}

	// 2. �ҵ�HOOK��λ��, ������5�ֽڵ�����
	// 2.1 HOOK��λ��ѡ��Ϊ(�˴�����5�ֽ�,):
	//  2be1            sub     esp, ecx ;
	// 	c1e902          shr     ecx, 2   ;
	UCHAR hookCode[5] = { 0x2b,0xe1,0xc1,0xe9,0x02 }; //����inline hook���ǵ�5�ֽ�
	ULONG i = 0;
	for (; i < 0x1FF; ++i)
	{
		if (RtlCompareMemory((UCHAR*)uKiFastCallEntry + i,
			hookCode,
			5) == 5)
		{
			break;
		}
	}
	if (i >= 0x1FF)
	{
		KdPrint(("��KiFastCallEntry������û���ҵ�HOOKλ��,����KiFastCallEntry�Ѿ���HOOK����\n"));
		uninstallHook();
		return;
	}


	g_hookAddr = uKiFastCallEntry + i;
	g_hookAddr_next_ins = g_hookAddr + 5;

	// 3. ��ʼinline hook
	UCHAR jmpCode[5] = { 0xe9 };// jmp xxxx
	disablePageWriteProtect();

	// 3.1 ������תƫ��
	// ��תƫ�� = Ŀ���ַ - ��ǰ��ַ - 5
	*(ULONG*)(jmpCode + 1) = (ULONG)myKiFastEntryHook - g_hookAddr - 5;

	// 3.2 ����תָ��д��
	RtlCopyMemory(uKiFastCallEntry + i,
		jmpCode,
		5);

	enablePageWriteProtect();
}

void uninstallHook()
{
	if (g_hookAddr)
	{

		// ��ԭʼ����д��.
		UCHAR srcCode[5] = { 0x2b,0xe1,0xc1,0xe9,0x02 };
		disablePageWriteProtect();

		// 3.1 ������תƫ��
		// ��תƫ�� = Ŀ���ַ - ��ǰ��ַ - 5

		_asm sti
		// 3.2 ����תָ��д��
		RtlCopyMemory(g_hookAddr,
			srcCode,
			5);
		_asm cli
		g_hookAddr = 0;
		enablePageWriteProtect();
	}

	if (g_pNewNtKernel)
	{
		ExFreePool(g_pNewNtKernel);
		g_pNewNtKernel = NULL;
	}
}


// SSDT���˺���.
ULONG SSDTFilter(ULONG index,/*������,Ҳ�ǵ��ú�*/
	ULONG tableAddress,/*��ĵ�ַ,������SSDT��ĵ�ַ,Ҳ������Shadow SSDT��ĵ�ַ*/
	ULONG funAddr/*�ӱ���ȡ���ĺ�����ַ*/)
{
	// �����SSDT��Ļ�
	if (tableAddress == KeServiceDescriptorTable.ServiceTableBase)
	{
		// �жϵ��ú�(190��ZwOpenProcess�����ĵ��ú�)
		if (index == 190)
		{
			// ������SSDT��ĺ�����ַ
			// Ҳ�������ں˵ĺ�����ַ.
			return g_pNewSSDTEntry->ServiceTableBase[190];
		}
	}
	// ���ؾɵĺ�����ַ
	return funAddr;
}

// inline hook KiFastCallEntry�ĺ���
void _declspec(naked) myKiFastEntryHook()
{

	_asm
	{
		pushad; // ѹջ�Ĵ���: eax,ecx,edx,ebx, esp,ebp ,esi, edi
		pushfd; // ѹջ��־�Ĵ���

		push edx; // �ӱ���ȡ���ĺ�����ַ
		push edi; // ��ĵ�ַ
		push eax; // ���ú�
		call SSDTFilter; // ���ù��˺���

		;// �����������֮��ջ�ؼ�����,ָ��pushad��
		;// 32λ��ͨ�üĴ���������ջ��,ջ�ռ䲼��Ϊ:
		;// [esp + 00] <=> eflag
		;// [esp + 04] <=> edi
		;// [esp + 08] <=> esi
		;// [esp + 0C] <=> ebp
		;// [esp + 10] <=> esp
		;// [esp + 14] <=> ebx
		;// [esp + 18] <=> edx <<-- ʹ�ú�������ֵ���޸����λ��
		;// [esp + 1C] <=> ecx
		;// [esp + 20] <=> eax
		mov dword ptr ds : [esp + 0x18] , eax;
		popfd; // popfdʱ,ʵ����edx��ֵ�ͻر��޸�
		popad;

		; //ִ�б�hook���ǵ�����ָ��
		sub esp, ecx;
		shr ecx, 2;
		jmp g_hookAddr_next_ins;
	}
}