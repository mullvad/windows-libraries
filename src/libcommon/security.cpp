#include "stdafx.h"
#include "security.h"
#include "error.h"
#include "memory.h"
#include <vector>

namespace common::security
{

void AdjustTokenPrivilege(HANDLE token, const std::wstring &privilege, bool enable)
{
	LUID privilegeLuid;

	auto status = LookupPrivilegeValueW(nullptr, privilege.c_str(), &privilegeLuid);
	THROW_GLE_IF(FALSE, status, "Resolve privilege LUID");

	TOKEN_PRIVILEGES privs;

	privs.PrivilegeCount = 1;
	privs.Privileges[0].Luid = privilegeLuid;
	privs.Privileges[0].Attributes = (enable ? SE_PRIVILEGE_ENABLED : 0);

	status = AdjustTokenPrivileges(token, FALSE, &privs, 0, nullptr, nullptr);
	const auto error = GetLastError();

	//
	// Terrible interface.
	//
	if (FALSE == status || ERROR_SUCCESS != error)
	{
		common::error::Throw("Adjust token privileges", error);
	}
}

void AdjustCurrentThreadTokenPrivilege(const std::wstring &privilege, bool enable)
{
	HANDLE token;

	common::memory::ScopeDestructor sd;

	if (FALSE == OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &token))
	{
		const auto error = GetLastError();

		THROW_UNLESS(ERROR_NO_TOKEN, error, "Acquire access token for current thread");

		THROW_GLE_IF(FALSE, ImpersonateSelf(SecurityImpersonation), "Impersonate self");

		sd += []
		{
			THROW_GLE_IF(FALSE, RevertToSelf(), "Revert impersonation");
		};

		const auto status = OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &token);

		THROW_GLE_IF(FALSE, status, "Acquire access token for current thread");
	}

	sd += [&token]
	{
		CloseHandle(token);
	};

	AdjustTokenPrivilege(token, privilege, enable);
}

void AdjustCurrentProcessTokenPrivilege(const std::wstring &privilege, bool enable)
{
	auto processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
	THROW_GLE_IF(NULL, processHandle, "Open handle to own process");

	common::memory::ScopeDestructor sd;

	sd += [&processHandle]()
	{
		CloseHandle(processHandle);
	};

	HANDLE processToken;

	const auto status = OpenProcessToken(processHandle, TOKEN_ADJUST_PRIVILEGES, &processToken);
	THROW_GLE_IF(FALSE, status, "Acquire access token for own process");

	sd += [&processToken]()
	{
		CloseHandle(processToken);
	};

	AdjustTokenPrivilege(processToken, privilege, enable);
}

void AddAdminToObjectDacl(const std::wstring &objectName, SE_OBJECT_TYPE objectType)
{
	std::vector<uint8_t> adminSidStorage(SECURITY_MAX_SID_SIZE);

	SID *adminSid = reinterpret_cast<SID *>(&adminSidStorage[0]);
	DWORD adminSidSize = static_cast<DWORD>(adminSidStorage.size());

	const auto createSidStatus = CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr, adminSid, &adminSidSize);

	THROW_GLE_IF(FALSE, createSidStatus, "Create SID for BUILTIN\\Administrators");

	PACL currentAcl;
	PSECURITY_DESCRIPTOR securityDescriptor;

	const auto getSecurityStatus = GetNamedSecurityInfoW
	(
		objectName.c_str(),
		objectType,
		DACL_SECURITY_INFORMATION,
		nullptr,
		nullptr,
		&currentAcl,
		nullptr,
		&securityDescriptor
	);

	THROW_UNLESS(ERROR_SUCCESS, getSecurityStatus, "Retrieve DACL for object");

	common::memory::ScopeDestructor sd;

	sd += [&securityDescriptor]()
	{
		LocalFree(reinterpret_cast<HLOCAL>(securityDescriptor));
	};

	EXPLICIT_ACCESSW ea = { 0 };

	ea.grfAccessPermissions = GENERIC_ALL;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea.Trustee.ptstrName = reinterpret_cast<LPWSTR>(adminSid);

	PACL updatedAcl;

	const auto setEntriesStatus = SetEntriesInAclW(1, &ea, currentAcl, &updatedAcl);

	THROW_UNLESS(ERROR_SUCCESS, setEntriesStatus, "Create updated DACL");

	sd += [&updatedAcl]()
	{
		LocalFree(reinterpret_cast<HLOCAL>(updatedAcl));
	};

	const auto setSecurityStatus = SetNamedSecurityInfoW
	(
		const_cast<LPWSTR>(objectName.c_str()),
		objectType,
		DACL_SECURITY_INFORMATION,
		nullptr,
		nullptr,
		updatedAcl,
		nullptr
	);

	THROW_UNLESS(ERROR_SUCCESS, setSecurityStatus, "Apply updated DACL")
}

}
