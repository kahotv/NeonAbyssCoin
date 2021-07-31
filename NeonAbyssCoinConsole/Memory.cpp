#include "Memory.h"
#include <Psapi.h>
#include <vector>
#include <string>
#include <TlHelp32.h>
#include "Util.h"

bool Memory::EnumAllMemoryBlocks(HANDLE hProcess, OUT std::vector<MEMORY_BASIC_INFORMATION>& memories) {
	// ��� hProcess Ϊ�����������
	if (hProcess <= 0)
		return false;

	// ��ʼ�� vector ����
	memories.clear();
	memories.reserve(200);

	// ��ȡ PageSize �͵�ַ����
	SYSTEM_INFO sysInfo = { 0 };
	GetSystemInfo(&sysInfo);
	/*
		typedef struct _SYSTEM_INFO {
		  union {
			DWORD dwOemId;							// �����Ա���
			struct {
			  WORD wProcessorArchitecture;			// ����ϵͳ��������ϵ�ṹ
			  WORD wReserved;						// ����
			} DUMMYSTRUCTNAME;
		  } DUMMYUNIONNAME;
		  DWORD     dwPageSize;						// ҳ���С��ҳ�汣���ͳ�ŵ������
		  LPVOID    lpMinimumApplicationAddress;	// ָ��Ӧ�ó����dll�ɷ��ʵ�����ڴ��ַ��ָ��
		  LPVOID    lpMaximumApplicationAddress;	// ָ��Ӧ�ó����dll�ɷ��ʵ�����ڴ��ַ��ָ��
		  DWORD_PTR dwActiveProcessorMask;			// ����������
		  DWORD     dwNumberOfProcessors;			// ��ǰ�����߼�������������
		  DWORD     dwProcessorType;				// ���������ͣ������Ա���
		  DWORD     dwAllocationGranularity;		// �����ڴ����ʼ��ַ������
		  WORD      wProcessorLevel;				// ����������
		  WORD      wProcessorRevision;				// �������޶�
		} SYSTEM_INFO, *LPSYSTEM_INFO;
	*/

	//�����ڴ�
	const char* p = (const char*)sysInfo.lpMinimumApplicationAddress;
	MEMORY_BASIC_INFORMATION  memInfo = { 0 };
	while (p < sysInfo.lpMaximumApplicationAddress) {
		// ��ȡ���������ڴ�黺�����ֽ���
		size_t size = VirtualQueryEx(
			hProcess,								// ���̾��
			p,										// Ҫ��ѯ�ڴ��Ļ���ַָ��
			&memInfo,								// �����ڴ����Ϣ�� MEMORY_BASIC_INFORMATION ����
			sizeof(MEMORY_BASIC_INFORMATION64)		// ��������С
		);
		if (size != sizeof(MEMORY_BASIC_INFORMATION64)) { break; }

		// ���ڴ����Ϣ׷�ӵ� vector ����β��
		memories.push_back(memInfo);

		// �ƶ�ָ��
		p += memInfo.RegionSize;
	}

	return memories.size() > 0;
}

std::vector<Memory::MemoryInfo> Memory::EnumMemory(HANDLE hProcess, DWORD qeuryType)
{
	//HANDLE hModuleSnap = INVALID_HANDLE_VALUE;

	std::vector<MemoryInfo> memlist_out;
	std::vector<MEMORY_BASIC_INFORMATION> memlist;
	if (EnumAllMemoryBlocks(hProcess, memlist))
	{
		for (size_t i = 0; i < memlist.size(); i++)
		{
			const auto& chunk = memlist[i];

			if (chunk.State == MEM_COMMIT)
			{
				if (qeuryType == 0 || (chunk.Protect & qeuryType) != 0)
				{
					MemoryInfo info;
					info.AllocateBase = (UINT64)chunk.AllocationBase;
					info.BaseAddress = (UINT64)chunk.BaseAddress;
					info.Size = chunk.RegionSize;
					memlist_out.emplace_back(info);
				}
			}
		}
	}
	return memlist_out;
}

bool Memory::ReadBytes(HANDLE hProcess, UINT64 addr, UINT32 len, std::vector<BYTE>& data)
{
	if (hProcess <= 0)
		return false;

	data.resize(len);

	SIZE_T readSize = 0;
	return TRUE == ReadProcessMemory(hProcess, (void*)addr, &data[0], len, &readSize);
}

BYTE Memory::ReadByte(HANDLE hProcess, UINT64 addr)
{
	std::vector<BYTE> v;
	if (ReadBytes(hProcess, addr, 1, v))
	{
		return v[0];
	}
	return 0;
}
DWORD Memory::ReadDWORD(HANDLE hProcess, UINT64 addr)
{
	std::vector<BYTE> v;
	if (ReadBytes(hProcess, addr, 4, v))
	{
		return *(DWORD*)v.data();
	}
	return 0;
}
UINT64 Memory::ReadQWORD(HANDLE hProcess, UINT64 addr)
{
	std::vector<BYTE> v;
	if (ReadBytes(hProcess, addr, 8, v))
	{
		return *(UINT64*)v.data();
	}
	return 0;
}
//���ڴ���ʽ
ULONG64 Memory::ReadExprQWORD(HANDLE hProcess, const std::string& expr)
{
	auto v = Util::SplitString(expr, '+');

	if (v.size() > 0)
	{
		ULONG64 addr = 0, tmpaddr = 0;
		for (size_t i = 0; i < v.size(); i++)
		{
			sscanf_s(v[i].c_str(), "%llX", &tmpaddr);
			addr = ReadQWORD(hProcess, addr + tmpaddr);
		}
		return addr;
	}

	return 0;
}

//д�ڴ�
bool Memory::WriteDWORD(HANDLE hProcess, UINT64 addr, DWORD val)
{
	SIZE_T writeSize = 0;
	return WriteProcessMemory(hProcess, (void*)addr, &val, 4, &writeSize);
}

//�����ڴ�
std::vector<UINT64> Memory::SearchBinary(HANDLE hProcess, UINT64 addr, DWORD len, const MemorySearch& search)
{
	std::vector<UINT64> addrlist;
	std::vector<BYTE> v;

	if (ReadBytes(hProcess, addr, len, v))
	{
		return search.Search(addr, v.data(), v.size());
	}

	return addrlist;
}