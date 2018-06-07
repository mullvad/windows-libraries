#include "stdafx.h"
#include "wmi.h"
#include "libcommon/com.h"

namespace common::wmi
{

_variant_t ComGetProperty(CComPtr<IWbemClassObject> obj, const std::wstring &name)
{
	_variant_t val;

	const auto status = obj->Get(name.c_str(), 0, &val, nullptr, nullptr);

	VALIDATE_COM(status, "Retrieve COM property value");

	return val;
}

_variant_t ComGetPropertyAlways(CComPtr<IWbemClassObject> obj, const std::wstring &name)
{
	auto val = ComGetProperty(obj, name);

	if (VT_EMPTY == V_VT(&val) || VT_NULL == V_VT(&val))
	{
		throw std::runtime_error("A required COM property value is empty.");
	}

	return val;
}

}
