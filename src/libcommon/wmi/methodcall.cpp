#include "stdafx.h"
#include "methodcall.h"
#include "libcommon/com.h"
#include "wmi.h"
#include <algorithm>

namespace common::wmi
{

void MethodCall::addArgument(const std::wstring &name, _variant_t value)
{
	m_arguments.emplace_back(Argument(name, value));
}

void MethodCall::addNullArgument(const std::wstring &name, CIMTYPE type)
{
	m_arguments.emplace_back(Argument(name, type));
}

_variant_t MethodCall::invoke(IConnection &connection, CComPtr<IWbemClassObject> instance, const std::wstring &methodName)
{
	std::for_each(m_arguments.begin(), m_arguments.end(), [&](const Argument &arg)
	{
		HRESULT status;

		if (arg.nullValue())
		{
			status = instance->Put(arg.name().c_str(), 0, nullptr, arg.type());
		}
		else
		{
			_variant_t &value = const_cast<variant_t &>(arg.value());

			status = instance->Put(arg.name().c_str(), 0, &value, 0);
		}

		VALIDATE_COM(status, "Register COM method argument");
	});

	_variant_t path;

	auto status = instance->Get(_bstr_t(L"__PATH"), 0, &path, nullptr, nullptr);

	VALIDATE_COM(status, "Get COM instance path");

	CComPtr<IWbemClassObject> result;

	status = connection.services()->ExecMethod(V_BSTR(&path), _bstr_t(methodName.c_str()), 0, nullptr, instance, &result, nullptr);

	VALIDATE_COM(status, "Execute COM method call");

	return WmiGetProperty(result, L"ReturnValue");
}

}
