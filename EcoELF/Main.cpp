#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <functional>
#include <string>
#include <chrono>
#include <thread>

void FindXRRuntimeServersAndExecute(const std::function<void(const PROCESSENTRY32 hProcessSnap)>& action)
{
	PROCESSENTRY32 processEntry{};
	processEntry.dwSize = sizeof(PROCESSENTRY32);

	const auto processSnapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (processSnapshotHandle == INVALID_HANDLE_VALUE) {
		std::cout << "Couldn't create a process snapshot." << std::endl;
		return;
	}

	if (!Process32First(processSnapshotHandle, &processEntry))
	{
		std::cout << "Couldn't get a process entry." << std::endl;
		if (!CloseHandle(processSnapshotHandle)) {
			std::cout << "Couldn't close the snapshot handle." << std::endl;
		}
		return;
	}
	do
	{
		if (std::wstring(processEntry.szExeFile) == L"xr_runtime_server.exe") {
			action(processEntry);
		}
	} while (Process32Next(processSnapshotHandle, &processEntry));

	CloseHandle(processSnapshotHandle);
}

int main()
{
	std::cout << "Start finding XR runtime servers." << std::endl;

	PROCESS_POWER_THROTTLING_STATE powerThrottlingState = { PROCESS_POWER_THROTTLING_CURRENT_VERSION,
					PROCESS_POWER_THROTTLING_EXECUTION_SPEED, PROCESS_POWER_THROTTLING_EXECUTION_SPEED };
	const auto powerThrottlingStateSize = sizeof(powerThrottlingState);

	while (true) {
		FindXRRuntimeServersAndExecute([&powerThrottlingState, &powerThrottlingStateSize](PROCESSENTRY32 processEntry)
			{
				const auto processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processEntry.th32ProcessID);
				if (processHandle == nullptr) {
					std::cout << "Failed to open process handle." << std::endl;
					return;
				}

				if (!SetProcessInformation(processHandle, ProcessPowerThrottling, &powerThrottlingState, powerThrottlingStateSize)) {
					std::cout << "Failed to set process information" << std::endl;
					return;
				}

				if (!SetPriorityClass(processHandle, IDLE_PRIORITY_CLASS)) {
					std::cout << "Failed to set a priority class" << std::endl;
					return;
				}

				if (!CloseHandle(processHandle)) {
					std::cout << "Failed to close the process handle." << std::endl;
					return;
				}
			});

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	return 0;
}
