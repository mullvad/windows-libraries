#include "stdafx.h"
#include "security.h"
#include "error.h"
#include "memory.h"
#include <vector>

using UniqueHandle = common::memory::UniqueHandle;

namespace common::security
{

void AdjustTokenPrivilege(HANDLE token, const std::wstring &privilege, bool enable)
{
	LUID privilegeLuid;

	auto status = LookupPrivilegeValueW(nullptr, privilege.c_str(), &privilegeLuid);

	if (FALSE == status)
	{
		THROW_WINDOWS_ERROR(GetLastError(), "Resolve privilege LUID");
	}

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
		THROW_WINDOWS_ERROR(error, "Adjust token privileges");
	}
}

void AdjustCurrentProcessTokenPrivilege(const std::wstring &privilege, bool enable)
{
	auto processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());

	if (nullptr == processHandle)
	{
		THROW_WINDOWS_ERROR(GetLastError(), "Open handle to own process");
	}

	common::memory::ScopeDestructor sd;

	sd += [&processHandle]()
	{
		CloseHandle(processHandle);
	};

	HANDLE processToken;

	const auto status = OpenProcessToken(processHandle, TOKEN_ADJUST_PRIVILEGES, &processToken);

	if (FALSE == status)
	{
		THROW_WINDOWS_ERROR(GetLastError(), "Acquire access token for own process");
	}

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

	if (FALSE == createSidStatus)
	{
		THROW_WINDOWS_ERROR(GetLastError(), "Create SID for BUILTIN\\Administrators");
	}

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

	if (ERROR_SUCCESS != getSecurityStatus)
	{
		THROW_WINDOWS_ERROR(getSecurityStatus, "Retrieve DACL for object");
	}

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

	if (ERROR_SUCCESS != setEntriesStatus)
	{
		THROW_WINDOWS_ERROR(setEntriesStatus, "Create updated DACL");
	}

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

	if (ERROR_SUCCESS != setSecurityStatus)
	{
		THROW_WINDOWS_ERROR(setSecurityStatus, "Apply updated DACL");
	}
}

bool IsLocalSystemUser(HANDLE processToken)
{
	DWORD returnLength = 0;

	if (0 != GetTokenInformation(processToken, TokenUser, nullptr, 0, &returnLength))
	{
		THROW_ERROR("GetTokenInformation did not return an error");
	}

	DWORD error = GetLastError();

	if (error != ERROR_INSUFFICIENT_BUFFER)
	{
		THROW_WINDOWS_ERROR(error, "GetTokenInformation: Expected ERROR_INSUFFICIENT_BUFFER");
	}

	std::vector<uint8_t> tokenInfoStorage(returnLength);

	if (0 != GetTokenInformation(processToken, TokenUser, tokenInfoStorage.data(), tokenInfoStorage.size(), &returnLength))
	{
		THROW_WINDOWS_ERROR(GetLastError(), "GetTokenInformation");
	}

	const auto tokenUser = reinterpret_cast<TOKEN_USER *>(tokenInfoStorage.data());

	std::vector<uint8_t> localSystemSidStorage(MAX_SID_SIZE);
	SID *localSystemSid = reinterpret_cast<SID *>(localSystemSidStorage.data());
	DWORD localSystemSidSize = static_cast<DWORD>(localSystemSidStorage.size());

	if (0 == CreateWellKnownSid(WinLocalSystemSid, nullptr, localSystemSid, &localSystemSidSize))
	{
		THROW_WINDOWS_ERROR(GetLastError(), "CreateWellKnownSid");
	}

	return 0 != EqualSid(tokenUser->User.Sid, localSystemSid);
}

UniqueHandle DuplicateSecurityContext(DWORD processId)
{
	auto processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);

	if (nullptr == processHandle)
	{
		THROW_WINDOWS_ERROR(GetLastError(), "Open process");
	}

	HANDLE processToken;

	auto status = OpenProcessToken(processHandle, TOKEN_READ | TOKEN_DUPLICATE, &processToken);

	if (0 == status)
	{
		THROW_WINDOWS_ERROR(GetLastError(), "Open process token");
	}

	HANDLE duplicatedToken;

	status = DuplicateTokenEx(processToken, MAXIMUM_ALLOWED, nullptr, SecurityImpersonation, TokenPrimary, &duplicatedToken);

	if (FALSE == status)
	{
		THROW_WINDOWS_ERROR(GetLastError(), "Duplicate token");
	}

	return UniqueHandle(new HANDLE(duplicatedToken));
}

}
