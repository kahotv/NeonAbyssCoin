#include "Memory.h"
#include <Psapi.h>
#include <vector>
#include <string>
#include <TlHelp32.h>
#include "Util.h"

bool Memory::EnumAllMemoryBlocks(HANDLE hProcess, OUT std::vector<MEMORY_BASIC_INFORMATION>& memories) {
	// 如果 hProcess 为空则结束运行
	if (hProcess <= 0)
		return false;

	// 初始化 vector 容量
	memories.clear();
	memories.reserve(200);

	// 获取 PageSize 和地址粒度
	SYSTEM_INFO sysInfo = { 0 };
	GetSystemInfo(&sysInfo);
	/*
		typedef struct _SYSTEM_INFO {
		  union {
			DWORD dwOemId;							// 兼容性保留
			struct {
			  WORD wProcessorArchitecture;			// 操作系统处理器体系结构
			  WORD wReserved;						// 保留
			} DUMMYSTRUCTNAME;
		  } DUMMYUNIONNAME;
		  DWORD     dwPageSize;						// 页面大小和页面保护和承诺的粒度
		  LPVOID    lpMinimumApplicationAddress;	// 指向应用程序和dll可访问的最低内存地址的指针
		  LPVOID    lpMaximumApplicationAddress;	// 指向应用程序和dll可访问的最高内存地址的指针
		  DWORD_PTR dwActiveProcessorMask;			// 处理器掩码
		  DWORD     dwNumberOfProcessors;			// 当前组中逻辑处理器的数量
		  DWORD     dwProcessorType;				// 处理器类型，兼容性保留
		  DWORD     dwAllocationGranularity;		// 虚拟内存的起始地址的粒度
		  WORD      wProcessorLevel;				// 处理器级别
		  WORD      wProcessorRevision;				// 处理器修订
		} SYSTEM_INFO, *LPSYSTEM_INFO;
	*/

	//遍历内存
	const char* p = (const char*)sysInfo.lpMinimumApplicationAddress;
	MEMORY_BASIC_INFORMATION  memInfo = { 0 };
	while (p < sysInfo.lpMaximumApplicationAddress) {
		// 获取进程虚拟内存块缓冲区字节数
		size_t size = VirtualQueryEx(
			hProcess,								// 进程句柄
			p,										// 要查询内存块的基地址指针
			&memInfo,								// 接收内存块信息的 MEMORY_BASIC_INFORMATION 对象
			sizeof(MEMORY_BASIC_INFORMATION64)		// 缓冲区大小
		);
		if (size != sizeof(MEMORY_BASIC_INFORMATION64)) { break; }

		// 将内存块信息追加到 vector 数组尾部
		memories.push_back(memInfo);

		// 移动指针
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
//读内存表达式
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

//写内存
bool Memory::WriteDWORD(HANDLE hProcess, UINT64 addr, DWORD val)
{
	SIZE_T writeSize = 0;
	return WriteProcessMemory(hProcess, (void*)addr, &val, 4, &writeSize);
}

//搜索内存
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