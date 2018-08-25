/*
 * Log.h
 *
 *  Created on: May 21, 2017
 *      Author: j
 */

#pragma once
#include <spdlog/spdlog.h>

namespace Util {

class Log {
public:
	~Log();

	static Log& instance();

	std::shared_ptr<spdlog::logger> get(){
		return logger;
	}

	void sysdie();

private:
	Log();

	std::shared_ptr<spdlog::logger> logger;
};

} /* namespace Util */

#define LogINFO(...) Util::Log::instance().get()->info(__VA_ARGS__)
#define LogERR(...) Util::Log::instance().get()->error(__VA_ARGS__)
#define LogDBG(...) Util::Log::instance().get()->debug(__VA_ARGS__)
#define LogSYSDIE Util::Log::instance().sysdie()
#define LogDIE(...) {Util::Log::instance().get()->error(__VA_ARGS__); std::runtime_error("die");}

