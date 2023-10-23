#pragma once

#include <string>
#include <windows.h>
#include <shlobj.h>
#include <filesystem>

namespace common::fs
{

void Mkdir(const std::wstring &path);

void CreatePrivilegedDirectory(std::filesystem::path path);

std::wstring GetKnownFolderPath(REFKNOWNFOLDERID folderId, DWORD flags = KF_FLAG_DEFAULT, HANDLE userToken = nullptr);

class ScopedNativeFileSystem
{
public:

	ScopedNativeFileSystem();
	~ScopedNativeFileSystem();

	ScopedNativeFileSystem(const ScopedNativeFileSystem &) = delete;
	ScopedNativeFileSystem &operator=(const ScopedNativeFileSystem &) = delete;

private:

	PVOID m_context;
};

}
