/*
 * Copyright (C) 2013 FOSS-Group
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
 * vm.cpp
 *
 *  Created on: 15.11.2013
 *      Author: cwi
 */

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/conversion.hpp>

#include "LDAPModList.h"

#include "include/cronConfiguration.hpp"
#include "include/logger.hpp"

using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

ostream& operator <<(ostream& s, const struct tm& tm);

enum others {O_NONE, O_DAY, O_HOUR, O_MINUTE};
enum params {P_NONE, P_DOW_OK};

time_t CronConfiguration::createTime() {
	time_t retval = 0;
	SYSLOGLOGGER(logDEBUG) << "CronConfiguration::createTime: " << *this;

	if (cronActive && 0 < cronDay.length() && 0 < cronDayOfWeek.length() && 0 < cronHour.length() &&
			0 < cronMinute.length() && 0 < cronMonth.length()) {
		if (0 != cronMinute.compare("*") && (0 == cronDay.compare("*") || 0 == cronDayOfWeek.compare("*"))) {
			time_t rawtime;
			struct tm * timelocal;
			struct tm timeinfo;
			time(&rawtime);
			timelocal = localtime(&rawtime);
			timelocal->tm_sec = 0;
			memcpy(&timeinfo, timelocal, sizeof(timeinfo));
			others setOthers = O_NONE;
			params setParams = P_NONE;

			ptime t = ptime_from_tm(timeinfo);
			SYSLOGLOGGER(logDEBUG) << "Orig:  " << timeinfo << endl;
			if (0 != cronMonth.compare("*")) {
				int month = atoi(cronMonth.c_str());
				if (month < timeinfo.tm_mon) {
					//t = t + years(1);
					t = t + months(12 - timeinfo.tm_mon);
				}
				else {
					t = t + months(month - timeinfo.tm_mon);
				}
				setOthers = O_DAY;
				timeinfo = to_tm(t);
				timeinfo.tm_mon = month;
				t = ptime_from_tm(timeinfo);
			}
			SYSLOGLOGGER(logDEBUG) << "Month: " << timeinfo << endl;
			if (O_NONE == setOthers) {
				if (0 != cronDay.compare("*")) {
					int day = atoi(cronDay.c_str());
					if (day < timeinfo.tm_mday) {
						t = t + months(1);
						setOthers = O_HOUR;
						timeinfo = to_tm(t);
					}
					timeinfo.tm_mday = day;
					t = ptime_from_tm(timeinfo);
				}
				else if (0 != cronDayOfWeek.compare("*")) {
					mktime(&timeinfo);
					SYSLOGLOGGER(logDEBUG) << "W Day:   " << timeinfo.tm_wday;
					SYSLOGLOGGER(logDEBUG) << "  DOW:   " << cronDayOfWeek;

					std::vector < std::string > daysofweek;
					boost::algorithm::split(daysofweek, cronDayOfWeek, boost::is_any_of(","), boost::algorithm::token_compress_off);
					int day;
					unsigned int i;
					for (i=0; i<daysofweek.size(); i++) {
						day = atoi(daysofweek[i].c_str());
						if (day >= timeinfo.tm_wday) {
							break;
						}
					}
					if (i == daysofweek.size()) {
						day = atoi(daysofweek[0].c_str());
						if (day < timeinfo.tm_wday) {
							t = t + days(7 - day);
						}
						else {
							t = t + days(day - timeinfo.tm_wday);
						}
					}
					else {
						t = t + days(day - timeinfo.tm_wday);
						setParams = P_DOW_OK;
					}
					setOthers = O_HOUR;
					timeinfo = to_tm(t);
					SYSLOGLOGGER(logDEBUG) << "W Day:   " << timeinfo;
				}
			}
			if (O_NONE == setOthers) {
				if (0 != cronHour.compare("*")) {
					int hour = atoi(cronHour.c_str());
					if (hour < timeinfo.tm_hour) {
						if (0 != cronHour.compare("*")) {
							t = t + months(1);
						}
						else {
							t = t + days(7);
						}
						setOthers = O_MINUTE;

						timeinfo = to_tm(t);
					}
					timeinfo.tm_hour = hour;
					t = ptime_from_tm(timeinfo);
				}
				SYSLOGLOGGER(logDEBUG) << "  Hour:  " << timeinfo;
			}
			if (O_NONE == setOthers) {
				int minute = atoi(cronMinute.c_str());
				if (minute < timeinfo.tm_min) {
					t = t + hours(1);
					timeinfo = to_tm(t);

					//t = ptime_from_tm(timeinfo);
				}
				timeinfo.tm_min = minute;
				SYSLOGLOGGER(logDEBUG) << "  Min:   " << timeinfo;
				nextTime = mktime(&timeinfo);
			}

			if (O_NONE != setOthers) {
				switch (setOthers) {
					case O_DAY:
						if (0 != cronDay.compare("*")) {
							int day = atoi(cronDay.c_str());
							timeinfo.tm_mday = day;
						}
						else if (0 != cronDayOfWeek.compare("*")) {
							mktime(&timeinfo);
							int day = atoi(cronDayOfWeek.c_str());
							if (day < timeinfo.tm_wday) {
								t = t + days(7 - timeinfo.tm_wday);
							}
							else {
								t = t + days(day - timeinfo.tm_wday);
							}
							timeinfo = to_tm(t);
						}
						SYSLOGLOGGER(logDEBUG) << "O_Day:   " << timeinfo;
					case O_HOUR:
						if (0 != cronHour.compare("*")) {
							int hour = atoi(cronHour.c_str());
							if (hour < timeinfo.tm_hour) {
								if (0 != cronDay.compare("*")) {
									t = t + months(1);
								}
								else if (P_DOW_OK != setParams){
									t = t + days(7);
								}
								timeinfo = to_tm(t);
							}
							timeinfo.tm_hour = hour;
						}
						SYSLOGLOGGER(logDEBUG) << "O_Hour:  " << timeinfo;
					case O_MINUTE:
						int minute = atoi(cronMinute.c_str());
						timeinfo.tm_min = minute;
						SYSLOGLOGGER(logDEBUG) << "O_MIN:  " << timeinfo;
				}
			}
			retval = nextTime = mktime(&timeinfo);
		}
		else if (0 == cronMinute.compare("*")) {
			SYSLOGLOGGER(logINFO) << "Vm::createTime failed: sstCronMinute must not be '*'!";
		}
		else {
			SYSLOGLOGGER(logINFO) << "Vm::createTime failed: one of sstCronDay or sstCronDayOfWeek must be '*'!";
		}
	}
	else {
		SYSLOGLOGGER(logINFO) << "Vm::createTime failed: not all values set!";
	}
	return retval;
}

ostream& operator <<(ostream& s, const CronConfiguration& cronConfiguration) {
	s << "       Cron: cronActive: " << cronConfiguration.cronActive << "; cron: " << cronConfiguration.cronMinute
			<< ", " << cronConfiguration.cronHour << ", " << cronConfiguration.cronDay << ", "
			<< cronConfiguration.cronMonth << ", " << cronConfiguration.cronDayOfWeek << std::endl;

	return s;
}

ostream& operator <<(ostream& s, const struct tm& tm) {
	s << tm.tm_mday << '.' << tm.tm_mon + 1 << '.'
			<< tm.tm_year + 1900 << " - " << tm.tm_hour
			<< ':' << tm.tm_min << " (" << tm.tm_wday << ")";
	return s;
}
