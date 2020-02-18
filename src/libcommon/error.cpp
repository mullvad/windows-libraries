#include "stdafx.h"
#include "error.h"
#include "string.h"
#include <exception>
#include <iomanip>
#include <sstream>
#include <cstring>

namespace common::error {

namespace
{

[[noreturn]] void ThrowFormatted(const char *msg)
{
	if (std::current_exception())
	{
		std::throw_with_nested(std::runtime_error(msg));
	}

	throw std::runtime_error(msg);
}

const char *IsolateFilename(const char *filepath)
{
	const auto slash = strrchr(filepath, '/');
	const auto backslash = strrchr(filepath, '\\');

	if (nullptr == slash && nullptr == backslash)
	{
		return filepath;
	}

	return max(slash, backslash) + 1;
}

} // anonymous namespace

std::string FormatWindowsError(DWORD errorCode)
{
	LPSTR buffer;

	auto status = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, errorCode, 0, (LPSTR)&buffer, 0, nullptr);

	if (0 == status)
	{
		std::stringstream ss;

		ss << "System error 0x" << std::setw(8) << std::setfill('0') << std::hex << errorCode;

		return ss.str();
	}

	auto result = common::string::TrimRight(std::string(buffer));
	LocalFree(buffer);

	return result;
}

void Throw(const char *operation, DWORD errorCode, const char *file, size_t line)
{
	std::stringstream ss;

	ss << operation << ": " << common::error::FormatWindowsError(errorCode)
		<< " (" << IsolateFilename(file) << ": " << line << ")";

	ThrowFormatted(ss.str().c_str());
}

void Throw(const char *message, const char *file, size_t line)
{
	std::stringstream ss;

	ss << message << " (" << IsolateFilename(file) << ": " << line << ")";

	ThrowFormatted(ss.str().c_str());
}

void UnwindException(const std::exception &err, std::shared_ptr<common::logging::ILogSink> logSink)
{
	logSink->error(err.what());

	try
	{
		std::rethrow_if_nested(err);
	}
	catch (const std::exception &innerErr)
	{
		UnwindException(innerErr, logSink);
	}
	catch (...)
	{
	}
}

}
