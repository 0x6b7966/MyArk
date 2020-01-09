#pragma once
#include <winioctl.h>
#define WM_FLUSHDRIVER WM_USER + 1001
#define WM_FLUSHPROCESS WM_USER + 1002
#define WM_FLUSHIDT WM_USER + 1003
#define WM_FLUSHGDT WM_USER + 1004
#define WM_FLUSHSSDT WM_USER + 1005

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

#pragma pack(1)
// �����ṹ��
typedef struct _DRIVER
{
	DWORD dwDllBase;		// ��ַ
	DWORD dwSize;			// ��С
	WCHAR Name[256];		// ������
	WCHAR FullDllName[256]; // ·��
	struct _DRIVER* Next;	// �������ģ���ǱߵĽṹ����Ҫ���ֽ���
}DRIVER, * PDRIVER;
#pragma pack()

#pragma pack(1)
typedef struct _PROCESS
{
	DWORD dwPID;			// ����ID
	DWORD dwPPID;			// ������ID
	CHAR Name[256];			// ������
	WCHAR FullDllName[256]; // ·��
	struct _PROCESS* Next;
}PROCESS, * PPROCESS;
#pragma pack()

#pragma pack(1)
typedef struct _THREAD
{
	DWORD dwTID;			// �߳�ID
	DWORD dwBasePriority;	// ���ȼ�
	DWORD dwStartAddress;	// �̵߳���ʼ��ַ
	DWORD dwStatus;			// ״̬
	struct _THREAD* Next;	//
}THREAD, * PTHREAD;
#pragma pack()

#pragma pack(1)
typedef struct _MODULE
{
	WCHAR FULLDLLNAME[256];	// ģ��·��
	DWORD dwStartAddress;	// ģ�����ַ
	DWORD dwSize;			// ģ���С
	struct _MODULE* Next;	//
}MODULE, * PMODULE;
#pragma pack()
// ȫ���豸��
extern HANDLE g_hDev;

#pragma pack(1)
typedef struct _ENUMFILES
{
	char FileOrDirectory;	// һ���ֽ����������ļ�����Ŀ¼
	WCHAR FileName[256];	// �ļ���
	LONGLONG Size;			// �ļ���С
	LARGE_INTEGER CreateTime;	// ����ʱ��
	LARGE_INTEGER ChangeTime;	// ����޸�ʱ��
	struct _ENUMFILES* Next;
}ENUMFILES, *PENUMFILES;
#pragma pack(1)

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
	WCHAR KeyName[256];		// ����
	WCHAR ValueName[256];   // ֵ������
	DWORD ValueType;		// ֵ������
	UCHAR Value[256];	    // ֵ
}REGISTER, *PREGISTER;
#pragma pack()