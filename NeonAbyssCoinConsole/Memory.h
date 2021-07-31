#pragma once

#include <vector>
#include <string>
#include <Windows.h>

class MemorySearch
{
private:
	struct SearchData
	{
		BYTE mask;		//����
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
					//�ҵ�һ��
					ret.push_back(base + i);
					i += n;
				}
				else
				{
					//û�ҵ�
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
			//�Ϸ����жϣ����������֡���ĸ���ո�
			if (!((c >= '0' && c <= '9') ||
				(c >= 'a' && c <= 'f') ||
				(c >= 'A' && c <= 'F') ||
				c == ' ' || c == '?')
				)
			{
				return "";
			}

			//�����ո�
			if (c == ' ')
				continue;

			if (c >= 'a' && c <= 'f')
			{
				c &= ~0x20;	//Сдת������д
			}

			ret.push_back(c);
		}

		return ret;
	}

	bool GeneralSearchData(const std::string& code, std::vector<SearchData>& sd)
	{
		//�Ƴ��ո���ĸת������д
		std::string tmpcode = FormatHexString(code);
		//��ʽ�жϣ�����Ϊ�ա�������ż�����ַ�
		if (tmpcode.empty() || tmpcode.size() % 1 != 0)
			return false;
		//�����ָת����SearchData
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

	/*ö��ָ�����������ڴ��
	assert(hProcess != nullptr);
	����:
	  hProcess:  Ҫö�ٵĽ���,��ӵ��PROCESS_QUERY_INFORMATIONȨ��
	  memories:  ����ö�ٵ����ڴ������
	����:
	  �ɹ�����true,ʧ�ܷ���false.

	  //��������������������������������
	  //��Ȩ����������ΪCSDN������(-: LYSM:-)����ԭ�����£���ѭCC 4.0 BY - SA��ȨЭ�飬ת���븽��ԭ�ĳ������Ӽ���������
	  //ԭ�����ӣ�https ://blog.csdn.net/Simon798/article/details/101431160
	*/
	static bool EnumAllMemoryBlocks(HANDLE hProcess, OUT std::vector<MEMORY_BASIC_INFORMATION>& memories);

	static std::vector<Memory::MemoryInfo> EnumMemory(HANDLE hProcess, DWORD qeuryType);

	static bool ReadBytes(HANDLE hProcess, UINT64 addr, UINT32 len, std::vector<BYTE>& data);

	static BYTE ReadByte(HANDLE hProcess, UINT64 addr);
	static DWORD ReadDWORD(HANDLE hProcess, UINT64 addr);
	static UINT64 ReadQWORD(HANDLE hProcess, UINT64 addr);
	//���ڴ���ʽ
	static ULONG64 ReadExprQWORD(HANDLE hProcess, const std::string& expr);

	//д�ڴ�
	static bool WriteDWORD(HANDLE hProcess, UINT64 addr, DWORD val);

	//�����ڴ�
	static std::vector<UINT64> SearchBinary(HANDLE hProcess, UINT64 addr, DWORD len, const MemorySearch& search);
};

