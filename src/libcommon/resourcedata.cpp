#include "stdafx.h"
#include "resourcedata.h"
#include "error.h"
#include <stdexcept>

namespace common::resourcedata {

BinaryResource LoadBinaryResource(HMODULE moduleHandle, uint32_t resourceId)
{
	auto resourceInformationBlock = FindResourceW(moduleHandle, MAKEINTRESOURCEW(resourceId), RT_RCDATA);

	THROW_GLE_IF(resourceInformationBlock, nullptr, "Locate resource information block");

	BinaryResource result;

	result.size = SizeofResource(moduleHandle, resourceInformationBlock);

	auto resourceHandle = LoadResource(moduleHandle, resourceInformationBlock);

	THROW_GLE_IF(resourceHandle, nullptr, "Load resource data");

	result.data = reinterpret_cast<uint8_t *>(LockResource(resourceHandle));

	if (nullptr == result.data)
	{
		throw std::runtime_error("Failed to lock resource");
	}

	return result;
}

}
