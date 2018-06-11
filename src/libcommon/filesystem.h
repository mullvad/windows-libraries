#pragma once

#include <string>

namespace common::fs
{

void Mkdir(const std::wstring &path);

std::wstring GetPath(const std::wstring &filepath);

std::wstring GetFilename(const std::wstring &filepath);

}
