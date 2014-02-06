/*
 * Copyright (C) 2012 FOSS-Group
 *                    Germany
 *                    http://www.foss-group.de
 *                    support@foss-group.de
 *
 * Authors:
 *  Christian Wittkowski <wittkowski@devroom.de>
 *
 * Licensed under the EUPL, Version 1.1 or – as soon they
 * will be approved by the European Commission - subsequent
 * versions of the EUPL (the "Licence");
 * You may not use this work except in compliance with the
 * Licence.
 * You may obtain a copy of the Licence at:
 *
 * https://joinup.ec.europa.eu/software/page/eupl
 *
 * Unless required by applicable law or agreed to in
 * writing, software distributed under the Licence is
 * distributed on an "AS IS" basis,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the Licence for the specific language governing
 * permissions and limitations under the Licence.
 *
 *
 */

/*
 * logger.hpp
 *
 *  Created on: 05.06.2012
 *      Author: cwi
 */

#ifndef LOGGER_HPP_
#define LOGGER_HPP_

#include <string>
#include <fstream>
#include <sstream>

extern "C" {
#include <syslog.h>
}

enum TLogLevel {
	logERROR = 3, logWARNING, logINFO = 6, logDEBUG
};

template<typename LoggerOutputPolicy>
class Logger {
public:
	Logger() {
	}
	;
	virtual ~Logger();
	std::ostringstream& get(TLogLevel level = logINFO) {
		//os << (level > logDEBUG ? level - logDEBUG : 0) << '\t';
		switch (level) {
			case logERROR: os << "ERROR" << '\t'; break;
			case logWARNING: os << "WARNING" << '\t'; break;
			case logINFO: os << "INFO" << '\t'; break;
			case logDEBUG: os << "DEBUG" << '\t'; break;
		}
		messageLevel = level;
		return os;
	}
	;
public:
	static TLogLevel& getReportLevel() {
		return reportLevel;
	}
	static void setReportLevel(TLogLevel level) {
		reportLevel = level;
	}

protected:
	std::ostringstream os;
	TLogLevel messageLevel;
	static TLogLevel reportLevel;
private:
	Logger(const Logger&);
	Logger& operator =(const Logger&);
};

template <typename LoggerOutputPolicy>
TLogLevel Logger<LoggerOutputPolicy>::reportLevel = logERROR;

template <typename LoggerOutputPolicy>
Logger<LoggerOutputPolicy>::~Logger(){
	if (messageLevel <= Logger<LoggerOutputPolicy>::getReportLevel()) {
		os << std::endl;
		LoggerOutputPolicy::Output(messageLevel, os.str());
	}
}

class LoggerOutputSyslog {
public:
	static void Output(const int level, const std::string& msg);
};
inline void LoggerOutputSyslog::Output(const int level, const std::string& msg) {
	syslog(LOG_MAKEPRI(LOG_DAEMON, level), "%s", msg.c_str());
}

typedef Logger<LoggerOutputSyslog> SyslogLogger;

#define SYSLOGLOGGER(level) \
	if (level > SyslogLogger::getReportLevel()) ; \
	else SyslogLogger().get(level) << "(" << __FILE__ << ":" <<__LINE__ << ")\t"


class LoggerOutputCout {
public:
	static void Output(const int level, const std::string& msg);
};
inline void LoggerOutputCout::Output(const int level, const std::string& msg) {
	std::cout << msg;
}

typedef Logger<LoggerOutputCout> CoutLogger;

#define COUTLOGGER(level) \
	if (level > CoutLogger::getReportLevel()) ; \
	else CoutLogger().get(level)


class LoggerOutputFile {
private:
public:
	static std::string&  FilePath() {
		static std::string filepath = "";
		return filepath;
	}

	static void Output(const int level, const std::string& msg);
};
inline void LoggerOutputFile::Output(const int level, const std::string& msg) {
	if (0 < FilePath().length()) {
		std::ofstream out(FilePath(), std::ios_base::app);
		time_t rawtime;
		time(&rawtime);
		std::string str_t = ctime(&rawtime);
		out << (str_t.substr(0, str_t.length() - 1)) << " " << msg;
		out.close();
	}
}

typedef Logger<LoggerOutputFile> FileLogger;

#define FILELOGGER(level) \
	if (level > FileLogger::getReportLevel()) ; \
	else FileLogger().get(level)


#endif /* LOGGER_HPP_ */
