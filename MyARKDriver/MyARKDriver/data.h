#pragma once
#include <ntdef.h>
// �����ṹ��
#pragma pack(1)
typedef struct _DRIVER
{
	ULONG DllBase;				// ��ַ
	ULONG Size;					// ��С
	char Name[256 * 2];			// ������
	char FullDllName[256 * 2];  // ·��
	struct _DRIVER* Next;
}DRIVER, * PDRIVER;
#pragma pack()

#pragma pack(1)
typedef struct _PROCESS
{
	ULONG dwPID;			   // ����ID
	ULONG dwPPID;			   // ������ID
	char Name[256];		       // ������
	char FullDllName[256 * 2]; // ·��
	struct _PROCESS* Next;
}PROCESS, * PPROCESS;
#pragma pack()

#pragma pack(1)
typedef struct _THREAD
{
	ULONG ulTID;			// �߳�ID
	ULONG ulBasePriority;	// ���ȼ�
	ULONG ulStartAddress;	// �̵߳���ʼ��ַ
	ULONG ulStatus;			// ״̬
	struct _THREAD* Next;	//
}THREAD, * PTHREAD;
#pragma pack()

#pragma pack(1)
typedef struct _MODULE
{
	char FullDllName[256 * 2];	// ģ��·��
	ULONG dwStartAddress;	// ģ�����ַ
	ULONG dwSize;			// ģ���С
	struct _MODULE* Next;	//
}MODULE, * PMODULE;
#pragma pack()
struct _PEB
{
	UCHAR InheritedAddressSpace;                                            //0x0
	UCHAR ReadImageFileExecOptions;                                         //0x1
	UCHAR BeingDebugged;
	UCHAR BitField;
	void* Mutant;                                                           //0x4
	void* ImageBaseAddress;                                                 //0x8
	struct _PEB_LDR_DATA* Ldr;
};
//0x30 bytes (sizeof)
struct _PEB_LDR_DATA
{
	ULONG Length;                                                           //0x0
	UCHAR Initialized;                                                      //0x4
	VOID* SsHandle;                                                         //0x8
	struct _LIST_ENTRY InLoadOrderModuleList;                               //0xc
	struct _LIST_ENTRY InMemoryOrderModuleList;                             //0x14
	struct _LIST_ENTRY InInitializationOrderModuleList;                     //0x1c
	VOID* EntryInProgress;                                                  //0x24
	UCHAR ShutdownInProgress;                                               //0x28
	VOID* ShutdownThreadId;                                                 //0x2c
};

typedef struct _LDR_DATA_TABLE_ENTRY {
	LIST_ENTRY InLoadOrderLinks;    //˫������
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	USHORT LoadCount;
	USHORT TlsIndex;
	union {
		LIST_ENTRY HashLinks;
		struct {
			PVOID SectionPointer;
			ULONG CheckSum;
		}s1;
	}u1;
	union {
		struct {
			ULONG TimeDateStamp;
		}s2;
		struct {
			PVOID LoadedImports;
		}s3;
	}u2;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

#pragma pack(1)
typedef struct _ENUMFILES
{
	char FileOrDirectory;	// һ���ֽ����������ļ�����Ŀ¼
	char FileName[256 * 2];	// �ļ���
	LONGLONG Size;			// �ļ���С
	LARGE_INTEGER CreateTime;	// ����ʱ��
	LARGE_INTEGER ChangeTime;	// ����޸�ʱ��
	struct _ENUMFILES* Next;
}ENUMFILES, * PENUMFILES;
#pragma pack()

#pragma pack(1)
typedef struct _IDT_ENTRY
{
	UINT16 uOffsetLow;			// ����͵�ַ
	UINT16 uSelector;			// ��ѡ����
	UINT8  uReverse_1;			// ����
	UINT8  uType : 4;			// �ж�����
	UINT8  StorageSegment : 1;  // Ϊ0���ж���
	UINT8  uDpl : 2;			// ��Ȩ��
	UINT8  uPresent : 1;		// ���δʹ���жϣ�����Ϊ0
	UINT16 uOffsetHigh;			// �������ߵ�ַƫ��
}IDT_ENTRY, * PIDT_ENTRY;
#pragma pack()

typedef struct _IDT_INFO
{
	UINT16 uIdtLimit;		// IDT��Χ
	UINT16 uLowIdtBase;		// IDT�ͻ�ַ
	UINT16 uHighIdtBase;	// IDT�߻�ַ
}IDT_INFO, *PIDT_INFO;

#pragma pack(1)
typedef struct _GDT_ENTRY
{
	UINT64 Limit0_15 : 16;	// ����
	UINT64 base0_23 : 24;	// ��ַ
	UINT64 TYPE : 4;		// ����
	UINT64 S : 1;			// S λ��(Ϊ0����ϵͳ�Σ�1�Ǵ���λ������ݶ�)
	UINT64 DPL : 2;			// DPL ��Ȩ��
	UINT64 P : 1;			// Pλ�� �δ���
 	UINT64 Limit16_19 : 4;	// ����
	UINT64 AVL : 1;			// AVL ϵͳ�������λ
	UINT64 O : 1;			// O
	UINT64 D_B : 1;			// D_B Ĭ�ϴ�С
	UINT64 G : 1;			// G ������
	UINT64 Base24_31 : 8;	// Base
}GDT_ENTRY, * PGDT_ENTRY;
#pragma pack()

typedef struct _GDT_INFO
{
	UINT16 uGdtlimit;		// GDT��Χ
	UINT16 uLowGdtBase;		// GDT�ͻ�ַ
	UINT16 uHighGdtBase;	// GDT�߻�ַ
}GDT_INFO, *PGDT_INFO;


typedef struct _SystemServiceDescriptorTable
{
	PVOID ServiceTableBase;
	PULONG ServiceCounterTableBase;
	ULONG NumberOfService;
	ULONG ParamTableBase;
}SystemServiceDescriptorTable, * PSystemServiceDescriptorTable;


#pragma pack(1)
typedef struct _SSDT
{
	ULONG Address;			// ���ػ�ַ
	ULONG SysCallIndex;		// ���ú�
}SSDT, * PSSDT;
#pragma pack()

#pragma pack(1)
typedef struct _REGISTER
{
	ULONG Type;				// ���ͣ�0 Ϊ���1Ϊֵ
	char KeyName[256 * 2];	// ����
	char ValueName[256 * 2];// ֵ������
	ULONG ValueType;		// ֵ������
	UCHAR Value[256];			// ֵ
}REGISTER, * PREGISTER;
#pragma pack()