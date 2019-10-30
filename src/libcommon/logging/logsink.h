#pragma once

#include "ilogsink.h"
#include <mutex>
#include <functional>

namespace common::logging
{

enum class Severity
{
	Error,
	Warning,
	Info,
	Trace
};

using LogTarget = std::function<void(Severity, const char *)>;

class LogSink : public ILogSink
{
public:

	LogSink(LogTarget target);

	LogSink(const LogSink &) = delete;
	LogSink &operator=(const LogSink &) = delete;
	LogSink(LogSink &&) = default;
	LogSink &operator=(LogSink &&) = default;

	void setTarget(LogTarget target);

	virtual void error(const char *msg) override;
	virtual void warning(const char *msg) override;
	virtual void info(const char *msg) override;
	virtual void trace(const char *msg) override;

private:

	void forward(Severity severity, const char *msg);

	std::mutex m_mutex;
	LogTarget m_target;
};

}
