#include "stdafx.h"
#include "registry.h"
#include "registrypath.h"
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

void DefaultMoveKey(const common::registry::RegistryPath &source, const common::registry::RegistryPath &destination)
{
	HKEY destHandle;

	auto status = RegCreateKeyExW(destination.key(), destination.subkey().c_str(), 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &destHandle, nullptr);

	THROW_CODE_UNLESS(ERROR_SUCCESS, status, "Create destination key");

	status = RegCopyTreeW(source.key(), source.subkey().c_str(), destHandle);

	RegCloseKey(destHandle);

	THROW_CODE_UNLESS(ERROR_SUCCESS, status, "Copy registry key");

	status = RegDeleteTreeW(source.key(), source.subkey().c_str());

	THROW_CODE_UNLESS(ERROR_SUCCESS, status, "Clean up source tree");

	status = RegDeleteKeyW(source.key(), source.subkey().c_str());

	THROW_CODE_UNLESS(ERROR_SUCCESS, status, "Clean up source key");
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

	THROW_CODE_UNLESS(ERROR_SUCCESS, status, "Create registry key");

	return std::make_unique<RegistryKey>(subkeyHandle);
}

//static
std::unique_ptr<RegistryKey> Registry::OpenKey(HKEY key, const std::wstring &subkey, bool writeAccess, RegistryView view)
{
	const auto accessFlags = CreateAccessFlags(writeAccess, view);

	HKEY subkeyHandle;

	const auto status = RegOpenKeyExW(key, subkey.c_str(), 0, accessFlags, &subkeyHandle);

	THROW_CODE_UNLESS(ERROR_SUCCESS, status, "Open registry key");

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
		THROW_CODE(status, "Delete registry key");
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

		THROW_CODE_UNLESS(ERROR_SUCCESS, openStatus, "Open registry key for deleting tree");

		status = RegDeleteTreeW(subkeyHandle, nullptr);

		RegCloseKey(subkeyHandle);
	}

	if (ERROR_SUCCESS != status
		&& ERROR_FILE_NOT_FOUND != status)
	{
		THROW_CODE(status, "Delete registry tree");
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

	THROW_CODE_UNLESS(ERROR_SUCCESS, status, "Open registry key for monitoring");

	RegistryMonitorArguments args;

	args.key = subkeyHandle;
	args.events = events;
	args.monitorTree = monitorTree;

	return std::make_unique<RegistryMonitor>(std::move(args));
}

// static
void Registry::MoveKey
(
	HKEY sourceKey,
	const std::wstring &sourceSubkey,
	HKEY destinationKey,
	const std::wstring &destinationSubkey,
	RegistryView view
)
{
	RegistryPath source(sourceKey, sourceSubkey);
	RegistryPath destination(destinationKey, destinationSubkey);

	if (RegistryView::Default == view)
	{
		DefaultMoveKey(source, destination);
		return;
	}

	const REGSAM viewFlag = (RegistryView::Force32 == view ? KEY_WOW64_32KEY : KEY_WOW64_64KEY);

	auto releaseHkey = [](HKEY *key)
	{
		if (nullptr != *key)
		{
			RegCloseKey(*key);
		}

		delete key;
	};

	std::unique_ptr<HKEY, decltype(releaseHkey)>
		sourceHandle(new HKEY(nullptr), releaseHkey),
		destinationHandle(new HKEY(nullptr), releaseHkey);

 	auto status = RegOpenKeyExW(source.key(), source.subkey().c_str(), 0, KEY_ALL_ACCESS | viewFlag, sourceHandle.get());

	THROW_CODE_UNLESS(ERROR_SUCCESS, status, "Open source key");

	status = RegCreateKeyExW(destination.key(), destination.subkey().c_str(), 0, nullptr, 0, \
		KEY_ALL_ACCESS | viewFlag, nullptr, destinationHandle.get(), nullptr);

	THROW_CODE_UNLESS(ERROR_SUCCESS, status, "Create destination key");

	status = RegCopyTreeW(*sourceHandle, nullptr, *destinationHandle);

	THROW_CODE_UNLESS(ERROR_SUCCESS, status, "Copy registry key");

	status = RegDeleteTreeW(*sourceHandle, nullptr);

	THROW_CODE_UNLESS(ERROR_SUCCESS, status, "Clean up source tree");

	status = RegDeleteKeyExW(source.key(), source.subkey().c_str(), viewFlag, 0);

	THROW_CODE_UNLESS(ERROR_SUCCESS, status, "Clean up source key");
}

}
