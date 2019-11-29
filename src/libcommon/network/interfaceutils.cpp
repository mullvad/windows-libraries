#include "stdafx.h"
#include "libcommon/network/interfaceutils.h"
#include "libcommon/error.h"
#include "libcommon/string.h"
#include <algorithm>

namespace common::network
{

InterfaceUtils::NetworkAdapter::NetworkAdapter(
	const NciContext &nci,
	std::shared_ptr<std::vector<uint8_t>> addressesBuffer,
	PIP_ADAPTER_ADDRESSES entry
)
	: m_addressesBuffer(addressesBuffer)
	, m_entry(entry)
{
	guid = common::string::ToWide(entry->AdapterName);

	try
	{
		//
		// FIXME:
		// Work around incorrect alias sometimes
		// being returned on Windows 8.
		//
		// Steps to reproduce:
		// 1. Install NDIS 6 TAP driver v9.00.00.21.
		// 2. Update driver to v9.24.2.601.
		// 3. Rename TAP adapter.
		//
		// GetAdaptersAddresses() returns a generic name
		// for the *first* adapter instead of the correct
		// one, whereas ConvertInterfaceAliasToLuid() and
		// ConvertInterfaceLuidToAlias() yield correct values.
		//

		IID guidObj = { 0 };
		if (S_OK != IIDFromString(&guid[0], &guidObj))
		{
			throw std::runtime_error("IIDFromString() failed");
		}

		alias = nci.getConnectionName(guidObj);
	}
	catch (const std::exception&)
	{
		alias = entry->FriendlyName;
	}

	name = entry->Description;
}

//static
std::set<InterfaceUtils::NetworkAdapter> InterfaceUtils::GetAllAdapters(ULONG family, ULONG flags)
{
	ULONG bufferSize = 0;

	auto status = GetAdaptersAddresses(family, flags, nullptr, nullptr, &bufferSize);

	THROW_UNLESS(ERROR_BUFFER_OVERFLOW, status, "Probe for adapter listing buffer size");

	// Memory is cheap, this avoids a looping construct.
	bufferSize *= 2;

	auto buffer = std::make_shared<std::vector<uint8_t>>(bufferSize);
	auto addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(&(*buffer)[0]);

	status = GetAdaptersAddresses(family, flags, nullptr, addresses, &bufferSize);

	THROW_UNLESS(ERROR_SUCCESS, status, "Retrieve adapter listing");

	std::set<NetworkAdapter> adapters;

	NciContext nci;

	for (auto it = addresses; nullptr != it; it = it->Next)
	{
		adapters.emplace(NetworkAdapter(nci, buffer, it));
	}

	return adapters;
}

//static
std::set<InterfaceUtils::NetworkAdapter> InterfaceUtils::GetAllAdapters()
{
	return GetAllAdapters(AF_INET, GAA_FLAG_SKIP_UNICAST | GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST);
}

//static
std::set<InterfaceUtils::NetworkAdapter>
	InterfaceUtils::GetTapAdapters(const std::set<InterfaceUtils::NetworkAdapter>& adapters)
{
	std::set<NetworkAdapter> tapAdapters;

	for (const auto& adapter : adapters)
	{
		static const wchar_t name[] = L"TAP-Windows Adapter V9";

		//
		// Compare partial name, because once you start having more TAP adapters
		// they're named "TAP-Windows Adapter V9 #2" and so on.
		//

		if (0 == adapter.name.compare(0, _countof(name) - 1, name))
		{
			tapAdapters.insert(adapter);
		}
	}

	return tapAdapters;
}

//static
void InterfaceUtils::AddDeviceIpAddresses(NET_LUID device, const std::vector<SOCKADDR_INET>& addresses)
{
	for (const auto& address : addresses)
	{
		MIB_UNICASTIPADDRESS_ROW row;
		InitializeUnicastIpAddressEntry(&row);

		row.InterfaceLuid = device;
		row.Address = address;

		THROW_UNLESS(NO_ERROR, CreateUnicastIpAddressEntry(&row), "Assign IP address on network interface");
	}
}

}
