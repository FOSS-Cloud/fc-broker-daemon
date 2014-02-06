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
 * vm.hpp
 *
 *  Created on: 15.11.2013
 *      Author: cwi
 */

#ifndef CRON_CONFIGURATION_HPP_
#define CRON_CONFIGURATION_HPP_

/*
objectclass ( sstObjectClass:57
    NAME 'sstCronObjectClass'
    SUP top AUXILIARY
    MUST ( sstCronMinute $ sstCronHour $ sstCronDay $ sstCronMonth $ sstCronDayOfWeek $ sstCronActive )
    MAY ( sstCronCommand ) )
*/
class CronConfiguration {
protected:
	bool set;
	bool cronActive;
	std::string cronDay;
	std::string cronDayOfWeek;
	std::string cronHour;
	std::string cronMinute;
	std::string cronMonth;
	time_t nextTime;

public:
	CronConfiguration() {
		set = false;
		cronActive = false;
		nextTime = 0;
	};
	virtual ~CronConfiguration() {};

	time_t createTime();

	time_t getNextTime() {
		return nextTime;
	}

	void setCronActive(const bool active) {
		this->cronActive = active;
		this->set = true;
	}

	void setCronDay(const std::string day) {
		this->cronDay = day;
		this->set = true;
	}

	const std::string getCronDay() const {
		return cronDay;
	}

	void setCronDayOfWeek(const std::string day) {
		this->cronDayOfWeek = day;
		this->set = true;
	}

	void setCronHour(const std::string hour) {
		this->cronHour = hour;
		this->set = true;
	}

	void setCronMinute(const std::string minute) {
		this->cronMinute = minute;
		this->set = true;
	}

	void setCronMonth(const std::string month) {
		this->cronMonth = month;
		this->set = true;
	}

	const bool isSet() const {
		return set;
	}

	friend std::ostream& operator <<(std::ostream& s, const CronConfiguration& configConfiguration);
};


#endif /* CRON_CONFIGURATION_HPP_ */
