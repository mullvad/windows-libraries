#pragma once

#include "logging/ilogsink.h"
#include <stdexcept>
#include <string>
#include <memory>
#include <windows.h>

#define THROW_WINDOWS_ERROR(errorCode, operation)\
{\
	::common::error::Throw(operation, errorCode, __FILE__, __LINE__);\
}\

#define THROW_ERROR(message)\
{\
	::common::error::Throw(message, __FILE__, __LINE__);\
}\

namespace common::error {

std::string FormatWindowsError(DWORD errorCode);

//
// Note: The errorCode argument is a system error code, and will be formatted as such.
// For custom error codes, embed them in the message and use the overload with fewer arguments.
//
[[noreturn]] void Throw(const char *operation, DWORD errorCode, const char *file, size_t line);
[[noreturn]] void Throw(const char *message, const char *file, size_t line);

void UnwindException(const std::exception &err, std::shared_ptr<common::logging::ILogSink> logSink);

}
