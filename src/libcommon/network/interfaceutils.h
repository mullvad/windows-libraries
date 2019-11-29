#pragma once

#include <string>
#include <set>
#include <vector>
#include <memory>
#include <cstdint>

// Secret include order to get most common networking structs/apis
// And avoiding compilation errors
#include <winsock2.h>
#include <windows.h>
#include <ws2def.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#include <netioapi.h>
// end

namespace common::network
{

class InterfaceUtils
{
	InterfaceUtils() = delete;

public:

	struct NetworkAdapter
	{
		std::wstring guid;
		std::wstring name;
		std::wstring alias;

		NetworkAdapter(
			std::wstring _guid,
			std::wstring _name,
			std::wstring _alias,
			PIP_ADAPTER_ADDRESSES entry,
			std::shared_ptr<std::vector<uint8_t>> addressesBuffer
		)
			: guid(_guid)
			, name(_name)
			, alias(_alias)
			, m_entry(entry)
			, m_addressesBuffer(addressesBuffer)
		{
		}

		bool operator<(const NetworkAdapter& rhs) const
		{
			return _wcsicmp(guid.c_str(), rhs.guid.c_str()) < 0;
		}

		const PIP_ADAPTER_ADDRESSES raw() const
		{
			return reinterpret_cast<PIP_ADAPTER_ADDRESSES>(&(*m_addressesBuffer)[0]);
		}

	private:

		PIP_ADAPTER_ADDRESSES m_entry;
		std::shared_ptr<std::vector<uint8_t>> m_addressesBuffer;
	};

	static std::set<NetworkAdapter> GetAllAdapters(ULONG family, ULONG flags);
	static std::set<NetworkAdapter> GetAllAdapters();
	static std::set<NetworkAdapter> GetTapAdapters(const std::set<NetworkAdapter>& adapters);

	static void AddDeviceIpAddresses(NET_LUID device, const std::vector<SOCKADDR_INET>& addresses);
};

}
