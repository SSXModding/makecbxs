#pragma once

// TODO, this will probably be included in mcommon...

#include <mutex>

struct StdoutSink : public mco::Sink {
	void Output(const std::string& message, mco::LogSeverity severity) {
		std::lock_guard<std::mutex> lock(mutex);

		// add color before messages
		switch(severity) {
			case mco::LogSeverity::Warning:
				std::cout << "\033[33;1m";
				break;

			case mco::LogSeverity::Error:
				std::cout << "\033[31;1m";
				break;

			// we don't color these
			case mco::LogSeverity::Verbose:
			case mco::LogSeverity::Info:
			default:
				break;
		}

		std::cout << message << "\033[0m" << '\n';
	}

   private:
	/**
	 * Mutex lock.
	 */
	std::mutex mutex;
};

