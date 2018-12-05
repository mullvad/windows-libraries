#include "stdafx.h"
#include "process.h"
#include "error.h"
#include <stdexcept>
#include <experimental/filesystem>

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
	using namespace std::experimental;

	const auto fspath = filesystem::path(path);

	if (false == fspath.is_absolute()
		|| false == fspath.has_filename())
	{
		throw std::runtime_error("Invalid path spec for subprocess");
	}

	const auto workingDir = fspath.parent_path();
	const auto quotedPath = std::wstring(L"\"").append(path).append(L"\"");

	STARTUPINFOW si = { sizeof(STARTUPINFOW) };
	PROCESS_INFORMATION pi = { 0 };

	const auto status = CreateProcessW(nullptr, const_cast<wchar_t *>(quotedPath.c_str()),
		nullptr, nullptr, FALSE, 0, nullptr, workingDir.c_str(), &si, &pi);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	THROW_GLE_IF(FALSE, status, "Launch subprocess");
}

void RunInContext(HANDLE securityContext, const std::wstring &path)
{
	using namespace std::experimental;

	const auto fspath = filesystem::path(path);

	if (false == fspath.is_absolute()
		|| false == fspath.has_filename())
	{
		throw std::runtime_error("Invalid path spec for subprocess");
	}

	const auto workingDir = fspath.parent_path();
	const auto quotedPath = std::wstring(L"\"").append(path).append(L"\"");

	STARTUPINFOW si = { sizeof(STARTUPINFOW) };
	PROCESS_INFORMATION pi = { 0 };

	const auto status = CreateProcessWithTokenW(securityContext, 0, nullptr,
		const_cast<wchar_t *>(quotedPath.c_str()), 0, nullptr, workingDir.c_str(), &si, &pi);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	THROW_GLE_IF(FALSE, status, "Launch subprocess");
}

}
