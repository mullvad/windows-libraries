#include "stdafx.h"
#include "libcommon/serialization/serializer.h"
#include "libcommon/serialization/deserializer.h"
#include "gtest/gtest.h"
#include <string>
#include <cstdint>

TEST(LibCommonSerialization, SerializeAndDeserialize)
{
	//
	// Compose
	//

	::common::serialization::Serializer s;

	s << (uint8_t)8;
	s << (uint16_t)16;
	s << (uint32_t)32;

	GUID guid =
	{
		1,
		2,
		3,
		{ 4, 5, 6, 7, 8, 9, 'A', 'B' }
	};

	s << guid;

	std::wstring str(L"meh");

	s << str.c_str();
	s << str;

	std::vector<std::wstring> sa;

	sa.push_back(L"foo");
	sa.push_back(L"bar");

	s << sa;

	//
	// Decompose
	//

	auto blob = s.blob();

	::common::serialization::Deserializer d(&blob[0], blob.size());

	ASSERT_EQ(8, d.decode<uint8_t>());
	ASSERT_EQ(16, d.decode<uint16_t>());
	ASSERT_EQ(32, d.decode<uint32_t>());
	ASSERT_EQ(guid, d.decode<GUID>());
	EXPECT_EQ(str, d.decode<std::wstring>());
	EXPECT_EQ(str, d.decode<std::wstring>());

	std::vector<std::wstring> readSa;
	d >> readSa;

	for (size_t i = 0; i < sa.size(); ++i)
	{
		EXPECT_EQ(sa[i], readSa[i]) << "string arrays differ at index " << i << ", expected: " << sa[i] << " actual: " << readSa[i];
	}
}

TEST(LibCommonSerialization, DeserializeWrongTypeFails)
{
	::common::serialization::Serializer s;

	s << (uint8_t)8;

	auto blob = s.blob();

	::common::serialization::Deserializer d(&blob[0], blob.size());

	ASSERT_THROW(d.decode<uint16_t>(), std::runtime_error);
}

TEST(LibCommonSerialization, DeserializingExcessivelyFails)
{
	::common::serialization::Serializer s;

	s << (uint8_t)8;

	auto blob = s.blob();

	::common::serialization::Deserializer d(&blob[0], blob.size());

	d.decode<uint8_t>();

	ASSERT_THROW(d.decode<uint8_t>(), std::runtime_error);
}

TEST(LibCommonSerialization, CanHandleEmptyString)
{
	::common::serialization::Serializer s;

	s << L"first";
	s << L"";
	s << L"third";

	auto blob = s.blob();

	::common::serialization::Deserializer d(&blob[0], blob.size());

	EXPECT_EQ(L"first", d.decode<std::wstring>());
	EXPECT_EQ(L"", d.decode<std::wstring>());
	EXPECT_EQ(L"third", d.decode<std::wstring>());
}

TEST(LibCommonSerialization, CanHandleEmptyStringInArray)
{
	::common::serialization::Serializer s;

	std::vector<std::wstring> sa;

	sa.push_back(L"first");
	sa.push_back(L"");
	sa.push_back(L"third");

	s << sa;

	auto blob = s.blob();

	::common::serialization::Deserializer d(&blob[0], blob.size());

	std::vector<std::wstring> readSa;
	d >> readSa;

	for (size_t i = 0; i < sa.size(); ++i)
	{
		EXPECT_EQ(sa[i], readSa[i]) << "string arrays differ at index " << i << ", expected: " << sa[i] << " actual: " << readSa[i];
	}
}
