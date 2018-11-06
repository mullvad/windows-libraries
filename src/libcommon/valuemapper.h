#pragma once

#include <algorithm>
#include <utility>
#include <vector>
#include <stdexcept>
#include <initializer_list>

namespace common
{

template<typename T, typename U>
class ValueMapper
{
public:

	using value_type = std::pair<T, U>;

	ValueMapper(std::initializer_list<value_type> values)
	{
		m_values.reserve(values.size());
		std::copy(values.begin(), values.end(), std::back_inserter(m_values));
	}

	U map(T t) const
	{
		auto it = std::find_if(m_values.begin(), m_values.end(), [&t](const value_type &tuple)
		{
			return t == tuple.first;
		});

		if (m_values.end() == it)
		{
			throw std::runtime_error("Could not map between values");
		}

		return it->second;
	}

private:

	std::vector<value_type> m_values;
};

}
