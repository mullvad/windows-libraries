#include "stdafx.h"
#include "registry.h"
#include <libcommon/error.h>
#include <stdexcept>

namespace
{

REGSAM CreateAccessFlags(bool writeAccess, common::registry::RegistryView view)
{
	using namespace common::registry;

	REGSAM accessFlags = KEY_READ | (writeAccess ? KEY_WRITE : 0);

	switch (view)
	{
		case RegistryView::Force32:
		{
			accessFlags |= KEY_WOW64_32KEY;
			break;
		}
		case RegistryView::Force64:
		{
			accessFlags |= KEY_WOW64_64KEY;
			break;
		}
	};

	return accessFlags;
}

} // anonymous namespace

namespace common::registry
{

//static
std::unique_ptr<RegistryKey> Registry::CreateKey(HKEY key, const std::wstring &subkey, RegistryView view)
{
	const auto accessFlags = CreateAccessFlags(true, view);

	HKEY subkeyHandle;

	const auto status = RegCreateKeyExW(key, subkey.c_str(), 0, nullptr, 0, accessFlags, nullptr, &subkeyHandle, nullptr);

	THROW_UNLESS(ERROR_SUCCESS, status, "Create registry key");

	return std::make_unique<RegistryKey>(subkeyHandle);
}

//static
std::unique_ptr<RegistryKey> Registry::OpenKey(HKEY key, const std::wstring &subkey, bool writeAccess, RegistryView view)
{
	const auto accessFlags = CreateAccessFlags(writeAccess, view);

	HKEY subkeyHandle;

	const auto status = RegOpenKeyExW(key, subkey.c_str(), 0, accessFlags, &subkeyHandle);

	THROW_UNLESS(ERROR_SUCCESS, status, "Open registry key");

	return std::make_unique<RegistryKey>(subkeyHandle);
}

//static
void Registry::DeleteKey(HKEY key, const std::wstring &subkey, RegistryView view)
{
	LSTATUS status = ERROR_ACCESS_DENIED;

	if (RegistryView::Default == view)
	{
		status = RegDeleteKeyW(key, subkey.c_str());
	}
	else
	{
		const DWORD viewFlag = (RegistryView::Force32 == view ? KEY_WOW64_32KEY : KEY_WOW64_64KEY);

		status = RegDeleteKeyExW(key, subkey.c_str(), viewFlag, 0);
	}

	if (ERROR_SUCCESS != status
		&& ERROR_FILE_NOT_FOUND != status)
	{
		common::error::Throw("Delete registry key", status);
	}
}

//static
void Registry::DeleteTree(HKEY key, const std::wstring &subkey, RegistryView view)
{
	LSTATUS status = ERROR_ACCESS_DENIED;

	if (RegistryView::Default == view)
	{
		status = RegDeleteTreeW(key, subkey.c_str());
	}
	else
	{
		DWORD accessFlags = KEY_ALL_ACCESS;

		accessFlags |= (RegistryView::Force32 == view ? KEY_WOW64_32KEY : KEY_WOW64_64KEY);

		HKEY subkeyHandle;

		const auto openStatus = RegOpenKeyExW(key, subkey.c_str(), 0, accessFlags, &subkeyHandle);

		THROW_UNLESS(ERROR_SUCCESS, openStatus, "Open registry key for deleting tree");

		status = RegDeleteTreeW(subkeyHandle, nullptr);

		RegCloseKey(subkeyHandle);
	}

	if (ERROR_SUCCESS != status
		&& ERROR_FILE_NOT_FOUND != status)
	{
		common::error::Throw("Delete registry tree", status);
	}
}

//static
std::unique_ptr<RegistryMonitor> Registry::MonitorKey
(
	HKEY key,
	const std::wstring &subkey,
	const std::vector<RegistryEventFlag> &events,
	bool monitorTree,
	RegistryView view
)
{
	const auto accessFlags = CreateAccessFlags(false, view);

	HKEY subkeyHandle;

	const auto status = RegOpenKeyExW(key, subkey.c_str(), 0, accessFlags, &subkeyHandle);

	THROW_UNLESS(ERROR_SUCCESS, status, "Open registry key for monitoring");

	RegistryMonitorArguments args;

	args.key = subkeyHandle;
	args.events = events;
	args.monitorTree = monitorTree;

	return std::make_unique<RegistryMonitor>(std::move(args));
}

}
