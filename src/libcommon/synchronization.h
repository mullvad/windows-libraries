#pragma once

#include <mutex>

namespace common::sync {

template<typename M = std::mutex>
class ScopeLock
{
public:

	ScopeLock(ScopeLock<M> &rhs) = delete;
	ScopeLock(ScopeLock<M> &&rhs) = delete;

	ScopeLock(M &mutex)
		: m_mutex(mutex)
	{
		m_mutex.lock();
	}

	~ScopeLock()
	{
		m_mutex.unlock();
	}

private:

	M &m_mutex;
};

}
