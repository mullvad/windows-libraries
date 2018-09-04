#include "stdafx.h"
#include "process.h"
#include "error.h"
#include <stdexcept>

#define PSAPI_VERSION 2
#include <psapi.h>

namespace common::process
{

DWORD GetProcessIdFromName(const std::wstring &processName, std::function<bool(const std::wstring &lhs, const std::wstring &rhs)> comp)
{
	// Allocate heap storage for 512 PIDs.
	std::vector<DWORD> pids(512);

	const DWORD bytesAvailable = static_cast<DWORD>(pids.size()) * sizeof(DWORD);
	DWORD bytesWritten;

	const auto enumStatus = K32EnumProcesses(&pids[0], bytesAvailable, &bytesWritten);

	THROW_GLE_IF(FALSE, enumStatus, "Acquire list of PIDs in the system");

	size_t numberProcesses = bytesWritten / sizeof(DWORD);
	pids.resize(numberProcesses);

	for (auto process : pids)
	{
		auto processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process);

		if (NULL == processHandle)
		{
			continue;
		}

		wchar_t foundName[512];
		DWORD foundNameSize = ARRAYSIZE(foundName);

		const auto queryStatus = QueryFullProcessImageNameW(processHandle, 0, foundName, &foundNameSize);

		CloseHandle(processHandle);

		if (0 == queryStatus)
		{
			continue;
		}

		if (comp(processName, foundName))
		{
			return process;
		}
	}

	throw std::runtime_error("Could not find named process");
}

}
