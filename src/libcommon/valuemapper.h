#pragma once

#include <algorithm>
#include <utility>
#include <vector>
#include <stdexcept>
#include <initializer_list>

namespace common
{

struct ValueMapper
{
	template<typename T, typename U, std::size_t S>
	static U map(T t, const std::pair<T, U> (&dictionary)[S])
	{
		for (const auto &entry : dictionary)
		{
			if (t == entry.first)
			{
				return entry.second;
			}
		}

		throw std::runtime_error("Could not map between values");
	}
};

}
