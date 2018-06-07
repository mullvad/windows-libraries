#pragma once

#include <atlbase.h>
#include <comutil.h>
#include <string>
#include <wbemidl.h>

namespace common::wmi
{

_variant_t WmiGetProperty(CComPtr<IWbemClassObject> obj, const std::wstring &name);
_variant_t WmiGetPropertyAlways(CComPtr<IWbemClassObject> obj, const std::wstring &name);

}
