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
 * evenlyPolicyInterval.hpp
 *
 *  Created on: 08.11.2013
 *      Author: cwi
 */

#ifndef EVENLYPOLICYINTERVAL_HPP_
#define EVENLYPOLICYINTERVAL_HPP_

extern "C" {
#include <limits.h>
}
#include "evenlyPolicy.hpp"
#include "vm.hpp"

class VirtTools;

class EvenlyPolicyInterval : public EvenlyPolicy {
private:
	int interval;
	int nextStart;

public:
	EvenlyPolicyInterval()
	: interval(-1)
	, nextStart(INT_MAX)
	{};
	virtual ~EvenlyPolicyInterval() {};

	virtual int checkPolicy(VmPool* vmPool, VmFactory* vmFactory, VirtTools* vt);
	virtual bool preStart(VmPool* vmPool, VmFactory* vmFactory);

	void setInterval(int interval_)
	{
		interval = interval_ - Config::getInstance()->getCycle();
		if (interval < 1)
		{
			interval = 1;
		}
		if (INT_MAX == nextStart) {
			nextStart = interval;
		}
	}

	void setNextStart(int nextStart_)
	{
		nextStart = nextStart_;
		if (nextStart < 1)
		{
			nextStart = -1;
		}
	}

	int getNextStart() {
		return nextStart;
	}

	friend std::ostream& operator <<(std::ostream& s, const EvenlyPolicyInterval& ep);

};

#endif /* EVENLYPOLICYINTERVAL_HPP_ */
