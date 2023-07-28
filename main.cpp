#include <windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <spdlog/spdlog.h>
#include "Driver/driver.hpp"

using QWORD = uint64_t;

HANDLE get_handle(const std::wstring& proc_name, DWORD access_mask)
{
	// Get a list of all processes
	HANDLE process_snapshot = CreateToolhelp32Snapshot(
		TH32CS_SNAPPROCESS,
		0
	);

	// ... and make sure it's valid
	if (process_snapshot == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	PROCESSENTRY32 process_entry = {}; process_entry.dwSize = sizeof(process_entry);
	Process32First(process_snapshot, &process_entry);

	// Loop over all the processes and compare their names
	do
	{
		// If we found it, close the handle and return true
		if (!_wcsicmp(proc_name.c_str(), process_entry.szExeFile))
		{
			CloseHandle(process_snapshot);
			return OpenProcess(access_mask, false, process_entry.th32ProcessID);
		}

	} while (Process32Next(process_snapshot, &process_entry));

	CloseHandle(process_snapshot);
	return 0;
}

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

int main()
{
	printf("[R6S Stutter Fix] by Archie (Discord: archie_uwu)");
	HANDLE game_handle = get_handle(L"RainbowSix.exe", PROCESS_SET_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION);
	if (!game_handle)
	{
		spdlog::info("Failed to find game process (not running?)");
		std::cin.get();
		return 0;
	}

	spdlog::info("Press INSERT to reset threads");

	while (GetProcessId(game_handle))
	{
		constexpr QWORD single_thread = 0x1;

		if (GetAsyncKeyState(VK_INSERT) & 1)
		{
			DWORD_PTR game_affinity = 0, system_affinity = 0;
			if (!GetProcessAffinityMask(game_handle, &game_affinity, &system_affinity))
			{
				spdlog::info("Failed to query current game affinity! (Error {})", GetLastError());
				continue;
			}

			// Set affinity to a single thread
			if (!SetProcessAffinityMask(game_handle, 0x1))
			{
				spdlog::info("Failed to set current game affinity to a single thread! (Error {})", GetLastError());
				continue;
			}

			spdlog::info("Game set to single-thread mode successfully!");

			Sleep(1000);

			// Set affinity to a single thread
			if (!SetProcessAffinityMask(game_handle, game_affinity))
			{
				spdlog::info("Failed to restore game affinity! (Error {})", GetLastError());
				continue;
			}

			spdlog::info("Threads reset successfully!");
		}

		Sleep(10);
	}

	return 0;
}