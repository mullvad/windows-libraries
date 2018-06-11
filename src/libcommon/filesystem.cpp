#include "stdafx.h"
#include "filesystem.h"
#include "string.h"
#include <stdexcept>

namespace common::fs
{

void Mkdir(const std::wstring &path)
{
    if (path.empty())
    {
        return;
    }

    auto dirs = common::string::Tokenize(path, L"/\\");

    //
    // Implicit path so there is no work to be performed.
    //
    if (0 == dirs.size())
    {
        return;
    }

    //
    // Only the volume is specified so ignore this request.
    // TODO: It would be more correct to verify whether the volume exists.
    //
    if (1 == dirs.size())
    {
        return;
    }

    std::wstring target = L"\\\\?\\";

    auto it = dirs.begin();

    target.append(*it++).push_back(L'\\');

    DWORD lastError = ERROR_SUCCESS;

    do
    {
        target.append(*it++).push_back(L'\\');

        //
        // The first few levels can fail with ERROR_ACCESS_DENIED or
        // ERROR_ALREADY_EXISTS but we keep going and check the status on the final node.
        //
        lastError = (CreateDirectoryW(target.c_str(), nullptr) ? ERROR_SUCCESS : GetLastError());
    }
    while (it != dirs.end());

    if (ERROR_SUCCESS != lastError && ERROR_ALREADY_EXISTS != lastError)
    {
        auto msg = std::string("Failed to create directory: ").append(common::string::ToAnsi(target));

        throw std::runtime_error(msg.c_str());
    }
}

std::wstring GetPath(const std::wstring &filepath)
{
    if (filepath.empty())
    {
        return L"";
    }

    //
    // Perhaps there's no filename included.
    //
    if (L'/' == *filepath.rbegin() || L'\\' == *filepath.rbegin())
    {
        return filepath;
    }

    auto lastSlash = filepath.find_last_of(L"/\\");

    //
    // Perhaps there's no path included.
    //
    if (std::wstring::npos == lastSlash)
    {
        return L"";
    }

    return filepath.substr(0, lastSlash + 1);
}

std::wstring GetFilename(const std::wstring &filepath)
{
    if (filepath.empty())
    {
        return L"";
    }

    auto lastSlash = filepath.find_last_of(L"/\\");

    //
    // Perhaps there's no path included.
    //
    if (std::wstring::npos == lastSlash)
    {
        return filepath;
    }

    return filepath.substr(lastSlash + 1);
}

}
