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
 * evenlyPolicyInterval.cpp
 *
 *  Created on: 08.11.2013
 *      Author: cwi
 */

#include <iostream>
#include <cstdio>
#include <algorithm>

#include "include/evenlyPolicyInterval.hpp"
#include "include/node.hpp"
#include "include/vm.hpp"
#include "include/vmPool.hpp"
#include "include/vmFactory.hpp"
#include "include/logger.hpp"

/* only for testing */
#include "include/factory.hpp"

using namespace std;

int EvenlyPolicyInterval::checkPolicy(VmPool* vmPool, VmFactory* vmFactory, VirtTools* vt)
{
	SYSLOGLOGGER(logDEBUG) << "EvenlyPolicyInterval::checkPolicy";

	return EvenlyPolicy::checkPolicy(vmPool, vmFactory, vt);
}

bool EvenlyPolicyInterval::preStart(VmPool* vmPool, VmFactory* vmFactory) {
	SYSLOGLOGGER(logDEBUG) << "EvenlyPolicyInterval::preStart: numberVmsToStart " << numberVmsToStart;
	bool retval = true;
	// node[0] smallest number of vms
	// node[n-1] largest number of vms
	// create missing prestarted vms on that nodes with less than the average number of vms
	if (0 < numberVmsToStart)
	{
		SYSLOGLOGGER(logDEBUG) << "EvenlyPolicyInterval::preStart: before " << nextStart;

		if (0 >= nextStart) {
			for (vector<VmPoolNodeWrapper*>::iterator itNodes = nodeWrappers.begin(); itNodes != nodeWrappers.end();)
			{
				VmPoolNodeWrapper* nodeWrapper = *itNodes++;
				if (nodeWrapper->getNumberVms() < maxVmPerNode && 0 < numberVmsToStart)
				{
					const Vm* gi = vmPool->getGoldenImage();
					if (NULL != gi) {
						Vm* vm = vmFactory->createInstance(gi, nodeWrapper->getNode());
						vmPool->addVm(vm);
					}
					numberVmsToStart--;
					break;
				}
			}
			nextStart = interval;
		}
		else {
			nextStart -= Config::getInstance()->getCycle();
		}
		SYSLOGLOGGER(logDEBUG) << "EvenlyPolicyInterval::preStart: after " << nextStart;
		retval = false;
	}
	return retval;
}


ostream& operator << (ostream& s, const EvenlyPolicyInterval& ep)
{
    s << "EvenlyPolicyInterval" << endl;
    s << "     interval: " << ep.interval << endl;
    s << "     nextStart: " << ep.nextStart << endl;
    s << static_cast<const EvenlyPolicy&>(ep) << endl;
	return s;
}
