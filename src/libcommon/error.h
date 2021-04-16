#pragma once

#include "logging/ilogsink.h"
#include <stdexcept>
#include <string>
#include <memory>
#include <sstream>
#include <windows.h>

#define THROW_ERROR_TYPE(type, message, ...)\
{\
	::common::error::Throw<type>(message, __FILE__, __LINE__, __VA_ARGS__);\
}\

#define THROW_WINDOWS_ERROR(errorCode, operation)\
{\
	::common::error::Throw(operation, errorCode, __FILE__, __LINE__);\
}\

#define THROW_ERROR(message)\
{\
	::common::error::Throw<std::runtime_error>(message, __FILE__, __LINE__);\
}\

namespace common::error {

class WindowsException : public std::runtime_error
{
public:

	WindowsException(const char *message, uint32_t errorCode);

	DWORD errorCode() const { return m_errorCode; }

private:

	uint32_t m_errorCode;
};

std::string FormatWindowsError(DWORD errorCode);
const char *IsolateFilename(const char *filepath);

template<typename ExceptionClass, class ...ArgTs>
[[noreturn]] void Throw(const char *message, const char *file, size_t line, ArgTs... args)
{
	std::stringstream ss;

	ss << message << " (" << IsolateFilename(file) << ": " << line << ")";

	const auto formattedMessage = ss.str();

	if (std::current_exception())
	{
		std::throw_with_nested(ExceptionClass(formattedMessage.c_str(), args...));
	}

	throw ExceptionClass(formattedMessage.c_str(), args...);
}

//
// Note: The errorCode argument is a system error code, and will be formatted as such.
// For custom error codes, embed them in the message and use the overload with fewer arguments.
//
[[noreturn]] void Throw(const char *operation, DWORD errorCode, const char *file, size_t line);

void UnwindException(const std::exception &err, std::shared_ptr<common::logging::ILogSink> logSink);

}
