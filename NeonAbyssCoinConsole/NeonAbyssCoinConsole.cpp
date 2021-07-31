// NeonAbyssCoinConsole.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <Windows.h>
#include <stdio.h>
#include <format>
#include "Memory.h"

//取游戏进程ID
DWORD GetGamePid()
{
	HWND hWindow = FindWindowW(L"UnityWndClass", L"Neon Abyss");
	if (hWindow != NULL)
	{
		DWORD pid = 0;
		GetWindowThreadProcessId(hWindow, &pid);

		return pid;
	}

	return 0;
}

//搜索人物基址
ULONG64 FindPlayerVar(DWORD pid)
{
	HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (h <= 0)
	{
		printf("OpenProcess失败: GetLastError() = %d\n", GetLastError());
		return 0;
	}

	auto memlist = Memory::EnumMemory(h, PAGE_EXECUTE_READWRITE);
	if (memlist.size() == 0)
	{
		CloseHandle(h);
		printf("EnumMemory失败: GetLastError() = %d\n", GetLastError());
		return 0;
	}

	//按照特征码偏移来搜寻
	/*
	* 55 48 8b ec 48 83 ec 30 48 8d 64 24 00 90 49 bb ?? ?? ?? ?? ?? ?? ?? ?? 41 ff d3 48 8b c8 83 38 00 48 8d 64 24 00 49 bb ?? ?? ?? ?? ?? ?? ?? ?? 41 ff d3 48 8b 40 20 48 8d 65 00 5d c3

2583EB956C0 - 55                    - push rbp
2583EB956C1 - 48 8B EC              - mov rbp,rsp
2583EB956C4 - 48 83 EC 30           - sub rsp,30 { 48 }
2583EB956C8 - 48 8D 64 24 00        - lea rsp,[rsp+00]
2583EB956CD - 90                    - nop
2583EB956CE - 49 BB 3057B93E58020000 - mov r11,000002583EB95730 { (-326416299) }
2583EB956D8 - 41 FF D3              - call r11
2583EB956DB - 48 8B C8              - mov rcx,rax
2583EB956DE - 83 38 00              - cmp dword ptr [rax],00 { 0 }
2583EB956E1 - 48 8D 64 24 00        - lea rsp,[rsp+00]
2583EB956E6 - 49 BB C057B93E58020000 - mov r11,000002583EB957C0 { (58394.28) }
2583EB956F0 - 41 FF D3              - call r11
2583EB956F3 - 48 8B 40 20           - mov rax,[rax+20]
2583EB956F7 - 48 8D 65 00           - lea rsp,[rbp+00]
2583EB956FB - 5D                    - pop rbp
2583EB956FC - C3                    - ret


base +DA0
base +e10
	*/
	std::string code = "55 48 8b ec 48 83 ec 30 48 8d 64 24 00 90 49 bb ?? ?? ?? ?? ?? ?? ?? ?? 41 ff d3 48 8b c8 83 38 00 48 8d 64 24 00 49 bb ?? ?? ?? ?? ?? ?? ?? ?? 41 ff d3 48 8b 40 20 48 8d 65 00 5d c3";
	MemorySearch ms(code);
	ULONG64 funcAddr = 0;
	for (const auto& mem : memlist)
	{
		std::vector<ULONG64> searchResult = Memory::SearchBinary(h, mem.AllocateBase, mem.Size, ms);
		if (searchResult.size() > 0)
		{
			for (const auto& addr : searchResult)
			{
				//找到了，用第一个
				funcAddr = addr;
				break;
			}
		}
	}

	//根据规则来读取人物基址
	ULONG64 playerAddr = 0;
	if (funcAddr != 0)
	{
		//寻找基址，再funcAddr里
		funcAddr = Memory::ReadQWORD(h, funcAddr + 0x10);
		if (funcAddr != 0)
		{
			//简单的判断一下子函数头
			if (Memory::ReadByte(h, funcAddr) == 0x55)
			{
				playerAddr = Memory::ReadQWORD(h, funcAddr + 0xA);
			}
		}
	}

	CloseHandle(h);
	return playerAddr;
}

//读取金币和霓虹币
void ShowPlayerInfo(DWORD pid, ULONG64 playerObjectVar)
{
	/*
	+38=金币
	+44=霓虹币
	*/

	HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (h <= 0)
	{
		printf("OpenProcess失败: GetLastError() = %d\n", GetLastError());
		return;
	}


	std::string expr = std::format("{:X}+38+18+30+20", playerObjectVar);
	ULONG64 playerInfoObject = Memory::ReadExprQWORD(h, expr);
	if (playerInfoObject != 0)
	{
		DWORD 金币 = Memory::ReadDWORD(h, playerInfoObject + 0x38);
		DWORD 霓虹币 = Memory::ReadDWORD(h, playerInfoObject + 0x44);

		printf("金币:\t%d\n", 金币);
		printf("霓虹币:\t%d\n", 霓虹币);
	}else
	{
		printf("读取人物信息失败\n");
	}

	CloseHandle(h);
}

//修改金币和霓虹币
void WritePlayerInfo(DWORD pid, ULONG64 playerObjectVar,DWORD 金币,DWORD 霓虹币)
{
	/*
+38=金币
+44=霓虹币
*/

	HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (h <= 0)
	{
		printf("OpenProcess失败: GetLastError() = %d\n", GetLastError());
		return;
	}


	std::string expr = std::format("{:X}+38+18+30+20", playerObjectVar);
	ULONG64 playerInfoObject = Memory::ReadExprQWORD(h, expr);
	if (playerInfoObject != 0)
	{
		Memory::WriteDWORD(h, playerInfoObject + 0x38,金币);
		Memory::WriteDWORD(h, playerInfoObject + 0x44,霓虹币);
	}
	else
	{
		printf("读取人物信息失败\n");
	}

	CloseHandle(h);
}


int main()
{
	do
	{
		DWORD pid = GetGamePid();
		if (pid == 0) 
		{
			printf("游戏未启动\n");
			break;
		}

		printf("游戏进程ID: %d\n", pid);

		ULONG64 player = FindPlayerVar(pid);
		if (player == 0)
		{
			printf("寻找人物基址失败\n");
			break;
		}

		printf("人物基址: 0x%llX\n", player);

		//显示人物信息
		printf("修改前:\n");
		ShowPlayerInfo(pid, player);
		//修改金币和霓虹币为9999
		WritePlayerInfo(pid, player, 9999, 9999);
		//显示人物信息
		printf("修改后:\n");
		ShowPlayerInfo(pid, player);
	} while (false);

	system("pause");
}

