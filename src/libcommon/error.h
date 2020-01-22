#pragma once

#include "logging/ilogsink.h"
#include <stdexcept>
#include <string>
#include <memory>
#include <windows.h>

#define THROW_IF(unwanted, actual, operation)\
if(actual == unwanted)\
{\
	::common::error::Throw(operation, actual, __FILE__, __LINE__);\
}\

#define THROW_UNLESS(expected, actual, operation)\
if(actual != expected)\
{\
	::common::error::Throw(operation, actual, __FILE__, __LINE__);\
}\

#define THROW_GLE_UNLESS(expected, actual, operation)\
if(actual != expected)\
{\
	::common::error::Throw(operation, GetLastError(), __FILE__, __LINE__);\
}\

#define THROW_GLE_IF(unwanted, actual, operation)\
if(actual == unwanted)\
{\
	::common::error::Throw(operation, GetLastError(), __FILE__, __LINE__);\
}\

#define THROW_GLE(operation)\
{\
	::common::error::Throw(operation, GetLastError(), __FILE__, __LINE__);\
}\

#define THROW_UNCONDITIONALLY(operation)\
{\
	::common::error::Throw(operation, __FILE__, __LINE__);\
}\

#define THROW_WITH_CODE(operation, code)\
{\
	::common::error::Throw(operation, code, __FILE__, __LINE__);\
}\

namespace common::error {

std::wstring FormatWindowsError(DWORD errorCode);
std::string FormatWindowsErrorPlain(DWORD errorCode);

[[noreturn]] void Throw(const char *operation, DWORD errorCode, const char *file, size_t line);
[[noreturn]] void Throw(const char *operation, const char *file, size_t line);

void UnwindException(const std::exception &err, std::shared_ptr<common::logging::ILogSink> logSink);

}
