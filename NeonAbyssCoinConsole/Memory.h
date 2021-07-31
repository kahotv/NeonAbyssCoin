#pragma once

#include <vector>
#include <string>
#include <Windows.h>

class MemorySearch
{
private:
	struct SearchData
	{
		BYTE mask;		//掩码
		BYTE val;
	};
public:
	MemorySearch(const std::string& code)
	{
		if (!GeneralSearchData(code, m_data))
			m_data.clear();
	}

	std::vector<ULONG64> Search(ULONG64 base, const BYTE* addr, DWORD len) const
	{
		std::vector<ULONG64> ret;

		if (!m_data.empty())
		{
			const SearchData* sd = m_data.data();
			const size_t sdlen = m_data.size();

			for (size_t i = 0; i < len;)
			{
				size_t n = 0;
				for (; n < sdlen; n++)
				{
					if ((addr[i + n] & sd[n].mask) != sd[n].val)
						break;
				}

				if (n == sdlen)
				{
					//找到一个
					ret.push_back(base + i);
					i += n;
				}
				else
				{
					//没找到
					i++;
				}
			}

		}

		return ret;
	}

private:
	std::string FormatHexString(const std::string& code)
	{
		std::string ret;

		for (char c : code)
		{
			//合法性判断，必须是数字、字母、空格
			if (!((c >= '0' && c <= '9') ||
				(c >= 'a' && c <= 'f') ||
				(c >= 'A' && c <= 'F') ||
				c == ' ' || c == '?')
				)
			{
				return "";
			}

			//跳过空格
			if (c == ' ')
				continue;

			if (c >= 'a' && c <= 'f')
			{
				c &= ~0x20;	//小写转换到大写
			}

			ret.push_back(c);
		}

		return ret;
	}

	bool GeneralSearchData(const std::string& code, std::vector<SearchData>& sd)
	{
		//移除空格、字母转换到大写
		std::string tmpcode = FormatHexString(code);
		//格式判断：不能为空、必须是偶数个字符
		if (tmpcode.empty() || tmpcode.size() % 1 != 0)
			return false;
		//两两分割，转换到SearchData
		sd.resize(tmpcode.size() / 2);

		for (size_t i = 0; i < tmpcode.size() / 2; i++)
		{
			auto& tmpsd = sd[i];
			char h = tmpcode[i * 2 + 0];
			char l = tmpcode[i * 2 + 1];

			tmpsd.val = 0x00;
			tmpsd.mask = 0xFF;

			if (h >= '0' && h <= '9')
				tmpsd.val = (h - '0') << 4;
			else if (h >= 'A' && h <= 'F')
				tmpsd.val = (h - 'A' + 0xA) << 4;
			else if (h == '?')
				tmpsd.mask &= 0b00001111;
			else
				return false;

			if (l >= '0' && l <= '9')
				tmpsd.val += l - '0';
			else if (l >= 'A' && l <= 'F')
				tmpsd.val += l - 'A' + 0xA;
			else if (l == '?')
				tmpsd.mask &= 0b11110000;
			else
				return false;
		}

		return true;
	}

private:
	std::vector<SearchData> m_data;
};

class Memory
{
public:
	struct MemoryInfo
	{
		UINT64 AllocateBase;
		UINT64 BaseAddress;
		UINT64 Size;
	};

	/*枚举指定进程所有内存块
	assert(hProcess != nullptr);
	参数:
	  hProcess:  要枚举的进程,需拥有PROCESS_QUERY_INFORMATION权限
	  memories:  返回枚举到的内存块数组
	返回:
	  成功返回true,失败返回false.

	  //————————————————
	  //版权声明：本文为CSDN博主「(-: LYSM:-)」的原创文章，遵循CC 4.0 BY - SA版权协议，转载请附上原文出处链接及本声明。
	  //原文链接：https ://blog.csdn.net/Simon798/article/details/101431160
	*/
	static bool EnumAllMemoryBlocks(HANDLE hProcess, OUT std::vector<MEMORY_BASIC_INFORMATION>& memories);

	static std::vector<Memory::MemoryInfo> EnumMemory(HANDLE hProcess, DWORD qeuryType);

	static bool ReadBytes(HANDLE hProcess, UINT64 addr, UINT32 len, std::vector<BYTE>& data);

	static BYTE ReadByte(HANDLE hProcess, UINT64 addr);
	static DWORD ReadDWORD(HANDLE hProcess, UINT64 addr);
	static UINT64 ReadQWORD(HANDLE hProcess, UINT64 addr);
	//读内存表达式
	static ULONG64 ReadExprQWORD(HANDLE hProcess, const std::string& expr);

	//写内存
	static bool WriteDWORD(HANDLE hProcess, UINT64 addr, DWORD val);

	//搜索内存
	static std::vector<UINT64> SearchBinary(HANDLE hProcess, UINT64 addr, DWORD len, const MemorySearch& search);
};

