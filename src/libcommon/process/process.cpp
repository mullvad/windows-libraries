#include "stdafx.h"
#include "process.h"
#include "../error.h"
#include "../string.h"
#include <stdexcept>
#include <filesystem>

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

	const auto msg = std::wstring(L"Could not find process with name \"").append(processName).append(L"\"");
	THROW_UNCONDITIONALLY(common::string::ToAnsi(msg).c_str());
}

std::unordered_set<DWORD> GetAllProcessIdsFromName(const std::wstring &processName,
	std::function<bool(const std::wstring &lhs, const std::wstring &rhs)> comp)
{
	std::unordered_set<DWORD> result;

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
			result.insert(process);
		}
	}

	return result;
}

void Run(const std::wstring &path)
{
	const auto fspath = std::filesystem::path(path);

	if (false == fspath.is_absolute()
		|| false == fspath.has_filename())
	{
		THROW_UNCONDITIONALLY("Invalid path specification for subprocess");
	}

	const auto workingDir = fspath.parent_path();
	const auto quotedPath = std::wstring(L"\"").append(path).append(L"\"");

	STARTUPINFOW si = { sizeof(STARTUPINFOW) };
	PROCESS_INFORMATION pi = { 0 };

	const auto status = CreateProcessW(nullptr, const_cast<wchar_t *>(quotedPath.c_str()),
		nullptr, nullptr, FALSE, 0, nullptr, workingDir.c_str(), &si, &pi);

	THROW_GLE_IF(FALSE, status, "Launch subprocess");

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
}

void RunInContext(HANDLE securityContext, const std::wstring &path)
{
	const auto fspath = std::filesystem::path(path);

	if (false == fspath.is_absolute()
		|| false == fspath.has_filename())
	{
		THROW_UNCONDITIONALLY("Invalid path specification for subprocess");
	}

	const auto workingDir = fspath.parent_path();
	const auto quotedPath = std::wstring(L"\"").append(path).append(L"\"");

	STARTUPINFOW si = { sizeof(STARTUPINFOW) };
	PROCESS_INFORMATION pi = { 0 };

	const auto status = CreateProcessWithTokenW(securityContext, 0, nullptr,
		const_cast<wchar_t *>(quotedPath.c_str()), 0, nullptr, workingDir.c_str(), &si, &pi);

	THROW_GLE_IF(FALSE, status, "Launch subprocess");

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
}

}
