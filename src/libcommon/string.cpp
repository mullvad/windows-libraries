#include "stdafx.h"
#include "string.h"
#include "memory.h"
#include "error.h"
#include <algorithm>
#include <iomanip>
#include <optional>
#include <memory>
#include <sddl.h>
#include <sstream>
#include <wchar.h>

namespace
{

struct ZeroGroupCount
{
	size_t firstIndex;
	int num;
};

std::optional<ZeroGroupCount> FindLongestZeroGroupsSequence(const std::vector<uint32_t> &ip)
{
	ZeroGroupCount longest = { 0 };
	ZeroGroupCount candidate = { 0 };

	const auto checkCandidate = [&]() {
		if (candidate.num > longest.num)
		{
			longest = candidate;
		}
		candidate = { 0 };
	};

	for (size_t index = 0; index < ip.size(); index++)
	{
		if (0 != ip[index])
		{
			checkCandidate();
		}
		else
		{
			if (0 != candidate.num)
			{
				candidate.num++;
			}
			else
			{
				candidate.firstIndex = index;
				candidate.num = 1;
			}
		}
	}

	checkCandidate();

	return longest.num > 0 ? std::make_optional(longest) : std::nullopt;
}

} // anonymous namespace

namespace common::string {

std::wstring FormatGuid(const GUID &guid)
{
	LPOLESTR buffer;

	auto status = StringFromCLSID(guid, &buffer);

	if (status != S_OK)
	{
		THROW_ERROR("Failed to format GUID");
	}

	std::wstring formatted(buffer);

	CoTaskMemFree(buffer);

	return formatted;
}

std::wstring FormatSid(const SID &sid)
{
	LPWSTR buffer;

	auto status = ConvertSidToStringSidW(const_cast<SID *>(&sid), &buffer);

	if (0 == status)
	{
		THROW_ERROR("Failed to format SID");
	}

	std::wstring formatted(buffer);

	LocalFree((HLOCAL)buffer);

	return formatted;
}

std::wstring Join(const std::vector<std::wstring> &parts, const std::wstring &delimiter)
{
	switch (parts.size())
	{
	case 0:
		return L"";
	case 1:
		return parts[0];
	default:
		{
			std::wstring joined;

			size_t reserveSize = 0;

			std::for_each(parts.begin(), parts.end(), [&reserveSize, &delimiter](const std::wstring &part)
			{
				reserveSize += (part.size() + delimiter.size());
			});

			joined.reserve(reserveSize);

			std::for_each(parts.begin(), parts.end(), [&joined, &delimiter](const std::wstring &part)
			{
				if (!joined.empty())
				{
					joined.append(delimiter);
				}

				joined.append(part);
			});

			return joined;
		}
	};
}

template<>
std::wstring FormatIpv4<AddressOrder::HostByteOrder>(uint32_t ip)
{
	std::wstringstream ss;

	ss << ((ip & 0xFF000000) >> 24) << L'.'
		<< ((ip & 0x00FF0000) >> 16) << L'.'
		<< ((ip & 0x0000FF00) >> 8) << L'.'
		<< ((ip & 0x000000FF));

	return ss.str();
}

template<>
std::wstring FormatIpv4<AddressOrder::NetworkByteOrder>(uint32_t ip)
{
	std::wstringstream ss;

	ss << ((ip & 0x000000FF)) << L'.'
		<< ((ip & 0x0000FF00) >> 8) << L'.'
		<< ((ip & 0x00FF0000) >> 16) << L'.'
		<< ((ip & 0xFF000000) >> 24);

	return ss.str();
}

std::wstring FormatIpv6(const uint8_t ip[16])
{
	constexpr uint32_t replacementSymbol = (uint32_t)-1;

	std::vector<uint32_t> ipArray;
	ipArray.insert(ipArray.end(), (uint16_t *)ip, ((uint16_t *)ip) + 8);

	//
	// Replace longest sequence of zero groups with special symbol
	//
	const auto zeroGroup = FindLongestZeroGroupsSequence(ipArray);

	if (zeroGroup.has_value())
	{
		if (zeroGroup->num == 8)
		{
			// Special case: all zeros
			return std::wstring(L"::");
		}

		if (zeroGroup->num > 1)
		{
			const auto startIter = ipArray.begin() + zeroGroup->firstIndex + 1;
			ipArray.erase(
				startIter,
				startIter + (zeroGroup->num - 1)
			);
		}
		ipArray[zeroGroup->firstIndex] = replacementSymbol;
	}

	//
	// Join into string
	//
	const auto swap = ::common::memory::ByteSwap;

	std::wstringstream ss;
	ss << std::hex;

	for (size_t i = 0; i < ipArray.size(); i++)
	{
		if (replacementSymbol == ipArray[i])
		{
			ss << L':';

			if (i + 1 == ipArray.size())
			{
				ss << L':';
			}
		}
		else
		{
			if (i > 0)
			{
				ss << L':';
			}
			ss << swap(static_cast<uint16_t>( ipArray[i] ));
		}
	}

	return ss.str();
}

std::wstring FormatIpv6(const uint8_t ip[16], uint8_t routingPrefix)
{
	std::wstringstream ss;

	ss << FormatIpv6(ip) << L"/" << routingPrefix;

	return ss.str();
}

std::wstring FormatTime(const FILETIME &filetime)
{
	FILETIME ft2;

	if (FALSE == FileTimeToLocalFileTime(&filetime, &ft2))
	{
		THROW_ERROR("Failed to convert time");
	}

	return FormatLocalTime(ft2);
}

std::wstring FormatLocalTime(const FILETIME &filetime)
{
	SYSTEMTIME st;

	if (FALSE == FileTimeToSystemTime(&filetime, &st))
	{
		THROW_ERROR("Failed to convert time");
	}

	std::wstringstream ss;

	ss << st.wYear << L'-'
		<< std::setw(2) << std::setfill(L'0') << st.wMonth << L'-'
		<< std::setw(2) << std::setfill(L'0') << st.wDay << L' '
		<< std::setw(2) << std::setfill(L'0') << st.wHour << L':'
		<< std::setw(2) << std::setfill(L'0') << st.wMinute << L':'
		<< std::setw(2) << std::setfill(L'0') << st.wSecond;

	return ss.str();
}

std::wstring Lower(const std::wstring &str)
{
	auto bufferSize = str.size() + 1;

	auto buffer = std::make_unique<wchar_t[]>(bufferSize);
	wcscpy_s(buffer.get(), bufferSize, str.c_str());

	_wcslwr_s(buffer.get(), bufferSize);

	return buffer.get();
}

std::vector<std::wstring> Tokenize(const std::wstring &str, const std::wstring &delimiters)
{
	auto bufferSize = str.size() + 1;

	auto buffer = std::make_unique<wchar_t[]>(bufferSize);
	wcscpy_s(buffer.get(), bufferSize, str.c_str());

	wchar_t *context = nullptr;

	auto token = wcstok_s(buffer.get(), delimiters.c_str(), &context);

	std::vector<std::wstring> tokens;

	while (token != nullptr)
	{
		tokens.push_back(token);
		token = wcstok_s(nullptr, delimiters.c_str(), &context);
	}

	return tokens;
}

std::vector<uint8_t> ToUtf8(const std::wstring &str)
{
	int rawStringLength = WideCharToMultiByte(
		CP_UTF8,
		0,
		str.c_str(),
		str.size(),
		nullptr,
		0,
		nullptr,
		nullptr
	);

	if (0 == rawStringLength)
	{
		THROW_WINDOWS_ERROR(GetLastError(), "WideCharToMultiByte");
	}

	std::vector<uint8_t> rawString(rawStringLength + 1);

	if (0 == WideCharToMultiByte(
		CP_UTF8,
		0,
		str.c_str(),
		str.size(),
		reinterpret_cast<char *>(rawString.data()),
		rawString.size() - 1,
		nullptr,
		nullptr
	))
	{
		THROW_WINDOWS_ERROR(GetLastError(), "WideCharToMultiByte");
	}

	return rawString;
}

std::string ToAnsi(const std::wstring &str)
{
	std::string ansi;

	ansi.reserve(str.size());

	std::transform(str.begin(), str.end(), std::back_inserter(ansi), [](wchar_t c)
	{
		return (c > 255 ? '?' : static_cast<char>(c));
	});

	return ansi;
}

std::wstring ToWide(const std::string &str)
{
	return std::wstring(str.begin(), str.end());
}

std::wstring Summary(const std::wstring &str, size_t max)
{
	if (str.size() <= max)
	{
		return str;
	}

	const wchar_t *padding = L"...";
	const size_t paddingLength = 3;

	if (max < paddingLength)
	{
		THROW_ERROR("Requested summary is too short");
	}

	auto summary = str.substr(0, max - paddingLength);
	summary.append(padding);

	return summary;
}

KeyValuePairs SplitKeyValuePairs(const std::vector<std::wstring> &serializedPairs)
{
	KeyValuePairs result;

	for (const auto &pair : serializedPairs)
	{
		auto index = pair.find(L'=');

		if (index == std::wstring::npos)
		{
			// Insert key with empty value.
			result.insert(std::make_pair(pair, L""));
		}
		else
		{
			result.insert(std::make_pair(
				pair.substr(0, index),
				pair.substr(index + 1)
			));
		}
	}

	return result;
}

const char *TrimChars = "\r\n\t ";
const wchar_t *WideTrimChars = L"\r\n\t ";

}
