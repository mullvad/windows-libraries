#include "stdafx.h"
#include "nci.h"
#include <libcommon/error.h>
#include <libcommon/filesystem.h>
#include <filesystem>
#include <stdexcept>

namespace common::network
{

Nci::Nci()
{
	const auto systemDir = common::fs::GetKnownFolderPath(
		FOLDERID_System,
		KF_FLAG_DEFAULT,
		nullptr
	);
	const auto nciPath = std::filesystem::path(systemDir).append(L"nci.dll");

	m_dllHandle = LoadLibraryW(nciPath.c_str());
	THROW_GLE_IF(nullptr, m_dllHandle, "Load nci.dll");

	m_nciGetConnectionName = reinterpret_cast<nciGetConnectionNameFunc>(
		GetProcAddress(m_dllHandle, "NciGetConnectionName"));

	if (nullptr == m_nciGetConnectionName)
	{
		FreeLibrary(m_dllHandle);
		throw std::runtime_error("Failed to obtain pointer to NciGetConnectionName");
	}

	m_nciSetConnectionName = reinterpret_cast<nciSetConnectionNameFunc>(
		GetProcAddress(m_dllHandle, "NciSetConnectionName"));

	if (nullptr == m_nciSetConnectionName)
	{
		FreeLibrary(m_dllHandle);
		throw std::runtime_error("Failed to obtain pointer to NciSetConnectionName");
	}
}

Nci::~Nci()
{
	FreeLibrary(m_dllHandle);
}

std::wstring Nci::getConnectionName(const GUID& guid) const
{
	DWORD nameLen = 0; // including L'\0'

	if (0 != m_nciGetConnectionName(&guid, nullptr, 0, &nameLen))
	{
		throw std::runtime_error("NciGetConnectionName() failed");
	}

	std::vector<wchar_t> buffer;
	buffer.resize(nameLen / sizeof(wchar_t));

	if (0 != m_nciGetConnectionName(&guid, &buffer[0], nameLen, nullptr))
	{
		throw std::runtime_error("NciGetConnectionName() failed");
	}

	return buffer.data();
}

void Nci::setConnectionName(const GUID& guid, const wchar_t* newName) const
{
	if (0 != m_nciSetConnectionName(&guid, newName))
	{
		throw std::runtime_error("NciSetConnectionName() failed");
	}
}

}
