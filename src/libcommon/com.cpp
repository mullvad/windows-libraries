#include "stdafx.h"
#include "com.h"
#include <algorithm>

namespace common
{

std::wstring ComConvertString(BSTR src)
{
	return std::wstring(src, SysStringLen(src));
}

std::vector<std::wstring> ComConvertStringArray(SAFEARRAY *src)
{
	CComSafeArray<BSTR> safeArray(src);

	std::vector<std::wstring> result;
	result.reserve(safeArray.GetCount());

	for (ULONG i = 0; i < safeArray.GetCount(); ++i)
	{
		result.emplace_back(ComConvertString(safeArray.GetAt(i)));
	}

	return result;
}

CComSafeArray<BSTR> ComConvertIntoStringArray(const std::vector<std::wstring> &src)
{
	CComSafeArray<BSTR> result;

	std::for_each(src.begin(), src.end(), [&](const std::wstring &str)
	{
		result.Add(_bstr_t(str.c_str()));
	});

	return result;
}

_variant_t ComPackageStringArray(CComSafeArray<BSTR> &src)
{
	VARIANT v;

	V_VT(&v) = VT_ARRAY | VT_BSTR;
	V_ARRAY(&v) = src.Detach();

	_variant_t vv;

	vv.Attach(v);

	return vv;
}

}
