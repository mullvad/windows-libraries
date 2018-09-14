#include "stdafx.h"
#include "libcommon/registry/registry.h"
#include "gmock/gmock.h"

using namespace common::registry;

namespace
{

HKEY g_regroot = HKEY_CURRENT_USER;
const wchar_t g_subkey[] = L"Software\\Amagicom-Test";

}

class LibCommonRegistryTest : public ::testing::Test
{
protected:

	void SetUp() override
	{
		ASSERT_NO_THROW(Registry::CreateKey(g_regroot, g_subkey));
	}

	void TearDown() override
	{
		ASSERT_NO_THROW(Registry::DeleteTree(g_regroot, g_subkey));
		ASSERT_NO_THROW(Registry::DeleteKey(g_regroot, g_subkey));
	}
};

TEST_F(LibCommonRegistryTest, OpenKey)
{
	// Read-only access.
	ASSERT_NO_THROW(Registry::OpenKey(g_regroot, g_subkey));

	// Read-write access.
	ASSERT_NO_THROW(Registry::OpenKey(g_regroot, g_subkey, true));
}

TEST_F(LibCommonRegistryTest, WriteReadStringValue)
{
	std::unique_ptr<RegistryKey> key;

	ASSERT_NO_THROW(key = Registry::OpenKey(g_regroot, g_subkey, true));

	const std::wstring valueName(L"StringValue");
	const std::wstring valueData(L"waffles");

	ASSERT_NO_THROW(key->writeValue(valueName, valueData));

	std::wstring readValueData;

	ASSERT_NO_THROW(readValueData = key->readString(valueName));

	ASSERT_STREQ(valueData.c_str(), readValueData.c_str());
}

TEST_F(LibCommonRegistryTest, WriteReadUint32Value)
{
	std::unique_ptr<RegistryKey> key;

	ASSERT_NO_THROW(key = Registry::OpenKey(g_regroot, g_subkey, true));

	const std::wstring valueName(L"Uint32Value");
	const uint32_t valueData(0xbeefcafe);

	ASSERT_NO_THROW(key->writeValue(valueName, valueData));

	uint32_t readValueData;

	ASSERT_NO_THROW(readValueData = key->readUint32(valueName));

	ASSERT_EQ(valueData, readValueData);
}

TEST_F(LibCommonRegistryTest, WriteReadUint64Value)
{
	std::unique_ptr<RegistryKey> key;

	ASSERT_NO_THROW(key = Registry::OpenKey(g_regroot, g_subkey, true));

	const std::wstring valueName(L"Uint64Value");
	const uint64_t valueData(0xbeefcafebeefbabe);

	ASSERT_NO_THROW(key->writeValue(valueName, valueData));

	uint64_t readValueData;

	ASSERT_NO_THROW(readValueData = key->readUint64(valueName));

	ASSERT_EQ(valueData, readValueData);
}

TEST_F(LibCommonRegistryTest, WriteReadBinaryBlobValue)
{
	std::unique_ptr<RegistryKey> key;

	ASSERT_NO_THROW(key = Registry::OpenKey(g_regroot, g_subkey, true));

	const std::wstring valueName(L"BinaryBlobValue");
	const std::vector<uint8_t> valueData
	{
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10
	};

	ASSERT_NO_THROW(key->writeValue(valueName, valueData));

	std::vector<uint8_t> readValueData;

	ASSERT_NO_THROW(readValueData = key->readBinaryBlob(valueName));

	ASSERT_EQ(valueData, readValueData);
}

TEST_F(LibCommonRegistryTest, WriteReadStringArrayValue)
{
	std::unique_ptr<RegistryKey> key;

	ASSERT_NO_THROW(key = Registry::OpenKey(g_regroot, g_subkey, true));

	const std::wstring valueName(L"StringArrayValue");
	const std::vector<std::wstring> valueData
	{
		L"three",
		L"blind",
		L"mice"
	};

	ASSERT_NO_THROW(key->writeValue(valueName, valueData));

	std::vector<std::wstring> readValueData;

	ASSERT_NO_THROW(readValueData = key->readStringArray(valueName));

	ASSERT_EQ(valueData, readValueData);
}

TEST_F(LibCommonRegistryTest, WriteDeleteValue)
{
	std::unique_ptr<RegistryKey> key;

	ASSERT_NO_THROW(key = Registry::OpenKey(g_regroot, g_subkey, true));

	const std::wstring valueName(L"dummy");

	ASSERT_NO_THROW(key->writeValue(valueName, valueName));

	ASSERT_NO_THROW(key->deleteValue(valueName));
}

TEST_F(LibCommonRegistryTest, EnumerateKeys)
{
	const std::vector<std::wstring> subkeys
	{
		L"one",
		L"two",
		L"three"
	};

	for (const auto &subkey : subkeys)
	{
		ASSERT_NO_THROW(Registry::CreateKey(g_regroot, std::wstring(g_subkey).append(L"\\").append(subkey)))
			<< "Create subkeys to have something to enumerate";
	}

	std::unique_ptr<RegistryKey> regkey;

	ASSERT_NO_THROW(regkey = Registry::OpenKey(g_regroot, g_subkey))
		<< "Open registry key for enumeration of subkeys";

	std::vector<std::wstring> foundKeys;

	auto callback = [&foundKeys](const std::wstring &subkey)
	{
		foundKeys.emplace_back(subkey);

		// Continue enumeration.
		return true;
	};

	ASSERT_NO_THROW(regkey->enumerateSubKeys(callback))
		<< "Enumerate subkeys";

	ASSERT_THAT(foundKeys, testing::UnorderedElementsAreArray(subkeys))
		<< "Set of found keys should match set of created keys";
}

TEST_F(LibCommonRegistryTest, EnumerateValues)
{
	const std::vector<std::wstring> values
	{
		L"one",
		L"two",
		L"three"
	};

	std::unique_ptr<RegistryKey> regkey;

	ASSERT_NO_THROW(regkey = Registry::OpenKey(g_regroot, g_subkey, true))
		<< "Open registry key";

	for (const auto &value : values)
	{
		ASSERT_NO_THROW(regkey->writeValue(value, L"dummy"))
			<< "Create registry values to have something to enumerate";
	}

	std::vector<std::wstring> foundValues;
	std::vector<uint32_t> foundTypes;

	auto callback = [&foundValues, &foundTypes](const std::wstring &valueName, uint32_t valueType)
	{
		foundValues.emplace_back(valueName);
		foundTypes.emplace_back(valueType);

		// Continue enumeration.
		return true;
	};

	ASSERT_NO_THROW(regkey->enumerateValues(callback))
		<< "Enumerate registry values";

	ASSERT_THAT(foundValues, testing::UnorderedElementsAreArray(values))
		<< "Set of found values should equal set of created values";

	ASSERT_THAT(foundTypes, testing::Each(uint32_t(REG_SZ)))
		<< "Found register values should have REG_SZ type";
}

TEST_F(LibCommonRegistryTest, MonitorValueChanges)
{
	std::unique_ptr<RegistryMonitor> monitor;

	std::vector<RegistryEventFlag> events
	{
		RegistryEventFlag::ValueChange
	};

	ASSERT_NO_THROW(monitor = Registry::MonitorKey(g_regroot, g_subkey, events))
		<< "Create key monitor";

	auto waitHandle = monitor->queueSingleEvent();

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Verify that wait handle is not signalled";

	std::unique_ptr<RegistryKey> key;

	ASSERT_NO_THROW(key = Registry::OpenKey(g_regroot, g_subkey, true))
		<< "Open key for writing";

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Opening key should not signal wait handle";

	const std::wstring valueName(L"StringValue");
	const std::wstring valueData(L"waffles");

	ASSERT_NO_THROW(key->writeValue(valueName, valueData));

	ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(waitHandle, 0))
		<< "Writing a value should signal the wait handle";

	waitHandle = monitor->queueSingleEvent();

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Queuing next event should return non-signalled wait handle";

	ASSERT_NO_THROW(Registry::CreateKey(g_regroot, std::wstring(g_subkey).append(L"\\g_subkey")))
		<< "Create sub key";

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Creating sub key should not signal wait handle";
}

TEST_F(LibCommonRegistryTest, MonitorKeyChanges)
{
	std::unique_ptr<RegistryMonitor> monitor;

	std::vector<RegistryEventFlag> events
	{
		RegistryEventFlag::SubkeyChange
	};

	ASSERT_NO_THROW(monitor = Registry::MonitorKey(g_regroot, g_subkey, events))
		<< "Create key monitor";

	auto waitHandle = monitor->queueSingleEvent();

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Verify that wait handle is not signalled";

	ASSERT_NO_THROW(Registry::CreateKey(g_regroot, std::wstring(g_subkey).append(L"\\g_subkey")))
		<< "Create sub key";

	ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(waitHandle, 0))
		<< "Creating sub key should signal wait handle";

	waitHandle = monitor->queueSingleEvent();

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Verify that rearmed wait handle is not signalled";

	ASSERT_NO_THROW(Registry::CreateKey(g_regroot, std::wstring(g_subkey).append(L"\\g_subkey\\moresub")))
		<< "Create sub-sub key";

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Creating sub-sub key should not signal wait handle";

	std::unique_ptr<RegistryKey> regkey;

	ASSERT_NO_THROW(regkey = Registry::OpenKey(g_regroot, g_subkey, true))
		<< "Open key for writing";

	ASSERT_NO_THROW(regkey->writeValue(L"dummy", L"dummy"))
		<< "Write value";

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Writing value should not signal wait handle";
}

TEST_F(LibCommonRegistryTest, MonitorKeyAndValueChanges)
{
	std::unique_ptr<RegistryMonitor> monitor;

	std::vector<RegistryEventFlag> events
	{
		RegistryEventFlag::SubkeyChange,
		RegistryEventFlag::ValueChange
	};

	ASSERT_NO_THROW(monitor = Registry::MonitorKey(g_regroot, g_subkey, events))
		<< "Create key monitor";

	auto waitHandle = monitor->queueSingleEvent();

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Verify that wait handle is not signalled";

	ASSERT_NO_THROW(Registry::CreateKey(g_regroot, std::wstring(g_subkey).append(L"\\g_subkey")))
		<< "Create sub key";

	ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(waitHandle, 0))
		<< "Creating sub key should signal wait handle";

	waitHandle = monitor->queueSingleEvent();

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Verify that rearmed wait handle is not signalled";

	std::unique_ptr<RegistryKey> regkey;

	ASSERT_NO_THROW(regkey = Registry::OpenKey(g_regroot, g_subkey, true))
		<< "Open key for writing";

	ASSERT_NO_THROW(regkey->writeValue(L"dummy", L"dummy"))
		<< "Write value";

	ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(waitHandle, 0))
		<< "Writing value should signal wait handle";
}

TEST_F(LibCommonRegistryTest, MonitorTreeKeyChanges)
{
	std::unique_ptr<RegistryMonitor> monitor;

	std::vector<RegistryEventFlag> events
	{
		RegistryEventFlag::SubkeyChange
	};

	ASSERT_NO_THROW(monitor = Registry::MonitorKey(g_regroot, g_subkey, events, true))
		<< "Create key monitor";

	auto waitHandle = monitor->queueSingleEvent();

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Verify that wait handle is not signalled";

	ASSERT_NO_THROW(Registry::CreateKey(g_regroot, std::wstring(g_subkey).append(L"\\g_subkey")))
		<< "Create sub key";

	ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(waitHandle, 0))
		<< "Creating sub key should signal wait handle";

	waitHandle = monitor->queueSingleEvent();

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Verify that rearmed wait handle is not signalled";

	ASSERT_NO_THROW(Registry::CreateKey(g_regroot, std::wstring(g_subkey).append(L"\\g_subkey\\moresub")))
		<< "Create sub-sub key";

	ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(waitHandle, 0))
		<< "Creating sub-sub key should signal wait handle";

	waitHandle = monitor->queueSingleEvent();

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Verify that rearmed wait handle is not signalled (2)";

	std::unique_ptr<RegistryKey> regkey;

	ASSERT_NO_THROW(regkey = Registry::OpenKey(g_regroot, g_subkey, true))
		<< "Open key for writing";

	ASSERT_NO_THROW(regkey->writeValue(L"dummy", L"dummy"))
		<< "Write value";

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Writing value should not signal wait handle";
}

TEST_F(LibCommonRegistryTest, MonitorTreeValueChanges)
{
	std::unique_ptr<RegistryMonitor> monitor;

	std::vector<RegistryEventFlag> events
	{
		RegistryEventFlag::ValueChange
	};

	ASSERT_NO_THROW(monitor = Registry::MonitorKey(g_regroot, g_subkey, events, true))
		<< "Create key monitor";

	auto waitHandle = monitor->queueSingleEvent();

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Verify that wait handle is not signalled";

	std::unique_ptr<RegistryKey> regkey;

	ASSERT_NO_THROW(regkey = Registry::CreateKey(g_regroot, std::wstring(g_subkey).append(L"\\g_subkey")))
		<< "Create sub key";

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Creating sub key should not signal wait handle";

	ASSERT_NO_THROW(regkey->writeValue(L"dummy", L"dummy"))
		<< "Write value in g_subkey";

	ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(waitHandle, 0))
		<< "Writing value in g_subkey should signal wait handle";

	waitHandle = monitor->queueSingleEvent();

	ASSERT_EQ(WAIT_TIMEOUT, WaitForSingleObject(waitHandle, 0))
		<< "Verify that rearmed wait handle is not signalled";

	ASSERT_NO_THROW(regkey = Registry::OpenKey(g_regroot, g_subkey, true))
		<< "Open key for writing";

	ASSERT_NO_THROW(regkey->writeValue(L"dummy", L"dummy"))
		<< "Write value";

	ASSERT_EQ(WAIT_OBJECT_0, WaitForSingleObject(waitHandle, 0))
		<< "Writing value should signal wait handle";
}

