#include "stdafx.h"
#include "logsink.h"

namespace common::logging
{

using LockType = std::scoped_lock<std::mutex>;

LogSink::LogSink(LogTarget target)
	: m_target(target)
{
}

void LogSink::setTarget(LogTarget target)
{
	LockType lock(m_mutex);
	m_target = target;
}

void LogSink::error(const char *msg)
{
	forward(Severity::Error, msg);
}

void LogSink::warning(const char *msg)
{
	forward(Severity::Warning, msg);
}

void LogSink::info(const char *msg)
{
	forward(Severity::Info, msg);
}

void LogSink::trace(const char *msg)
{
	forward(Severity::Trace, msg);
}

void LogSink::forward(Severity severity, const char *msg)
{
	LockType lock(m_mutex);

	if (m_target)
	{
		m_target(severity, msg);
	}
}

}
