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
 * http://www.osor.eu/eupl
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
 * evenlyPolicy.cpp
 *
 *  Created on: 18.05.2012
 *      Author: cwi
 */

#include <iostream>
#include <cstdio>
#include <algorithm>

#include "include/evenlyPolicy.hpp"
#include "include/node.hpp"
#include "include/vm.hpp"
#include "include/vmPool.hpp"
#include "include/vmFactory.hpp"
#include "include/logger.hpp"

/* only for testing */
#include "include/factory.hpp"

using namespace std;

bool NodeWrapperComp (VmPoolNodeWrapper* left,VmPoolNodeWrapper* right)
{
	return (left->getNumberVms() < right->getNumberVms());
}

int EvenlyPolicy::checkPolicy(VmPool* vmPool, VmFactory* vmFactory, VirtTools* vt) const
{
	// sort the nodes by count of active vms
	// calculate the average count of vms per node
	// count the number of unused vms
	// invoke the missing vms on the nodes with less than average-0.5 vms
	// sort the nodes by count of active vms

	if (0 == vmPool->getNodeWrappers()->size())
	{
		return -1;
	}
	vector<VmPoolNodeWrapper*> nodeWrappers;
	int numberVms = 0;
	int numberPrestartedVms = 0;
	for (map<string, VmPoolNodeWrapper*>::const_iterator it = vmPool->getNodeWrappers()->begin(); it != vmPool->getNodeWrappers()->end();)
	{
		VmPoolNodeWrapper* nodeWrapper = it->second;
		it++;
		for (map<string, Vm*>::const_iterator itVm = vmPool->getVms()->begin(); itVm != vmPool->getVms()->end();)
		{
			Vm* vm = itVm->second;
			itVm++;
			if (VmRunning == vm->getStatus() && vm->getNodeName() == nodeWrapper->getName())
			{
				numberVms++;
				if (!vm->isAssignedToUser())
				{
					numberPrestartedVms++;
				}
			}
		}
		nodeWrappers.push_back(nodeWrapper);
	}
	SYSLOGLOGGER(logDEBUG) << "numberPrestartedVms: " << numberPrestartedVms;
	SYSLOGLOGGER(logDEBUG) << "numberVms: " << numberVms;

	int numberVmsToStart = max(preStartNumberOfVirtualMachines - numberPrestartedVms, minimalNumberOfVirtualMachines - numberVms);
	SYSLOGLOGGER(logDEBUG) << "numberVmsToStart: " << numberVmsToStart;
	if (0 < numberVmsToStart)
	{
		numberVms += numberVmsToStart;
		int minVmPerNode = numberVms / static_cast<int>(nodeWrappers.size());
		int maxVmPerNode = minVmPerNode;
		if (0 < (numberVms % nodeWrappers.size()))
		{
			maxVmPerNode++;
		}
		SYSLOGLOGGER(logDEBUG) << "min: " << minVmPerNode << ", max: " << maxVmPerNode;

		// sort nodes by number vms
		std::sort(nodeWrappers.begin(), nodeWrappers.end(), NodeWrapperComp);
		// node[0] smallest number of vms
		// node[n-1] largest number of vms
		// create missing prestarted vms on that nodes with less than the average number of vms
		while (0 < numberVmsToStart)
		{
			for (vector<VmPoolNodeWrapper*>::iterator itNodes = nodeWrappers.begin(); itNodes != nodeWrappers.end();)
			{
				VmPoolNodeWrapper* nodeWrapper = *itNodes++;
				while (nodeWrapper->getNumberVms() < maxVmPerNode && 0 < numberVmsToStart)
				{
					// prestart one vm on this node
// CWI
					Vm* vm = vmFactory->createInstance(vmPool->getGoldenImage(), nodeWrapper->getNode());
					vmPool->addVm(vm);
					numberVmsToStart--;
				}
			}
		}
	}
	// number of nodes < 2 OR number of Vms < 2 -> nothing to optimize!
	if (2 > vmPool->getNodeWrappers()->size() || 2 > vmPool->getVms()->size())
	{
		return -1;
	}
	SYSLOGLOGGER(logDEBUG) << "let's start";
	while (true)
	{
		// sort nodes by number vms
		sort(nodeWrappers.begin(), nodeWrappers.end(), NodeWrapperComp);
		VmPoolNodeWrapper* nwSource = nodeWrappers.back();
		VmPoolNodeWrapper* nwDest = nodeWrappers.front();
		SYSLOGLOGGER(logDEBUG) << "source[n] " << nwSource->getName() << " " << nwSource->getNumberVms() <<
				     " dest[0] " << nwDest->getName() << " " << nwDest->getNumberVms();

		if (nwSource->getNumberVms() > (nwDest->getNumberVms() + tolerance))
		{
			SYSLOGLOGGER(logDEBUG) << "source:" << nwSource->getName() << " dest:" << nwDest->getName();
			nwSource->migrateFirstVm(nwDest, vt);
		}
		else
		{
			// the difference of the maximum vms in a node and the minimum vms in a node
			// is lower or equal than the tolerance value ->  leave the loop
			break;
		}
	}

//	for (vector<VmPoolNodeWrapper*>::iterator itNodes = nodeWrappers.begin(); itNodes != nodeWrappers.end();)
//	{
//		VmPoolNodeWrapper* nodeWrapper = *itNodes++;
//		SYSLOGLOGGER(logDEBUG) << "Nodes:" << nodeWrapper->getName();
//		for (set<Vm*>::iterator itVms = nodeWrapper->getVms()->begin(); itVms != nodeWrapper->getVms()->end();)
//		{
//			Vm* vm = *itVms++;
//			SYSLOGLOGGER(logDEBUG) << "Vm:" << vm->getName() << endl;
//		}
//	}
	return 0;
}

int EvenlyPolicy::checkPreStartPolicyTest(VmPool* vmPool, VmFactory* vmFactory) const
{
	// sort the nodes by count of active vms
	// calculate the average count of vms per node
	// count the number of unused vms
	// invoke the missing vms on the nodes with less than average-0.5 vms
	// sort the nodes by count of active vms

	SYSLOGLOGGER(logINFO) << "checkPreStartPolicy()";
	if (0 == vmPool->getNodeWrappers()->size())
	{
		return -1;
	}
	vector<VmPoolNodeWrapper*> nodeWrappers;
	int numberVms = 0;
	int numberPrestartedVms = 0;
	Factory<Vm> vmFactory_("Vm");
	for (map<string, VmPoolNodeWrapper*>::const_iterator it = vmPool->getNodeWrappers()->begin(); it != vmPool->getNodeWrappers()->end();)
	{
		VmPoolNodeWrapper* nodeWrapper = it->second;
		it++;
		for (map<string, Vm*>::const_iterator itVm = vmPool->getVms()->begin(); itVm != vmPool->getVms()->end();)
		{
			Vm* vm = itVm->second;
			itVm++;
			if (VmRunning == vm->getStatus() && vm->getNodeName() == nodeWrapper->getName())
			{
//				nodeWrapper->addVm(vm);
				numberVms++;
				if (!vm->isAssignedToUser())
				{
					numberPrestartedVms++;
				}
			}
		}
		nodeWrappers.push_back(nodeWrapper);
	}

	int numberVmsToStart = max(preStartNumberOfVirtualMachines - numberPrestartedVms, minimalNumberOfVirtualMachines - numberVms);
	if (0 < numberVmsToStart)
	{
		numberVms += numberVmsToStart;
		int minVmPerNode = numberVms / static_cast<int>(nodeWrappers.size());
		int maxVmPerNode = minVmPerNode;
		if (0 < (numberVms % nodeWrappers.size()))
		{
			maxVmPerNode++;
		}
		SYSLOGLOGGER(logDEBUG) << "min:" << minVmPerNode << " max:" << maxVmPerNode << endl;

		// sort nodes by number vms
		std::sort(nodeWrappers.begin(), nodeWrappers.end(), NodeWrapperComp);
		// node[0] smallest number of vms
		// node[n-1] largest number of vms
		// create missing prestarted vms on that nodes with less than the average number of vms
		while (0 < numberVmsToStart)
		{
			for (vector<VmPoolNodeWrapper*>::iterator itNodes = nodeWrappers.begin(); itNodes != nodeWrappers.end();)
			{
				VmPoolNodeWrapper* nodeWrapper = *itNodes++;
				while (nodeWrapper->getNumberVms() < maxVmPerNode && 0 < numberVmsToStart)
				{
					// prestart one vm on this node
					Vm* vm = vmFactory->createInstance(vmPool->getGoldenImage(), nodeWrapper->getNode());

//					Vm* vm = vmFactory_.createInstance();
//					vm->setNodeName(nodeWrapper->getNode()->getName());

					vmPool->addVm(vm);
					numberVmsToStart--;
				}
			}
		}
	}
	// number of nodes < 2 OR number of Vms < 2 -> nothing to optimize!
	if (2 > vmPool->getNodeWrappers()->size() || 2 > vmPool->getVms()->size())
	{
		return -1;
	}

	for (vector<VmPoolNodeWrapper*>::iterator itNodes = nodeWrappers.begin(); itNodes != nodeWrappers.end();)
	{
		VmPoolNodeWrapper* nodeWrapper = *itNodes++;
		SYSLOGLOGGER(logDEBUG) << "Nodes:" << nodeWrapper->getName();
		for (set<Vm*>::iterator itVms = nodeWrapper->getVms()->begin(); itVms != nodeWrapper->getVms()->end();)
		{
			Vm* vm = *itVms++;
			SYSLOGLOGGER(logDEBUG) << "Vm:" << vm->getName();
		}
	}
	return 0;
}

int EvenlyPolicy::checkEvenlyPolicyTest(VmPool* vmPool, VirtTools* vt) const
{
	// sort the nodes by count of active vms
	// calculate the average count of vms per node
	// count the number of unused vms
	// invoke the missing vms on the nodes with less than average-0.5 vms
	// sort the nodes by count of active vms

	SYSLOGLOGGER(logDEBUG) << "checkEvelnyPolicyTest()";
	if (0 == vmPool->getNodeWrappers()->size())
	{
		return -1;
	}
	vector<VmPoolNodeWrapper*> nodeWrappers;
	int numberVms = 0;
	int numberPrestartedVms = 0;
	for (map<string, VmPoolNodeWrapper*>::const_iterator it = vmPool->getNodeWrappers()->begin(); it != vmPool->getNodeWrappers()->end();)
	{
		VmPoolNodeWrapper* nodeWrapper = it->second;
		it++;
		for (map<string, Vm*>::const_iterator itVm = vmPool->getVms()->begin(); itVm != vmPool->getVms()->end();)
		{
			Vm* vm = itVm->second;
			itVm++;
			if (VmRunning == vm->getStatus() && vm->getNodeName() == nodeWrapper->getName())
			{
//				nodeWrapper->addVm(vm);
				numberVms++;
				if (!vm->isAssignedToUser())
				{
					numberPrestartedVms++;
				}
			}
		}
		nodeWrappers.push_back(nodeWrapper);
	}

	// number of nodes < 2 OR number of Vms < 2 -> nothing to optimize!
	if (2 > vmPool->getNodeWrappers()->size() || 2 > vmPool->getVms()->size())
	{
		return -1;
	}
	SYSLOGLOGGER(logDEBUG) << "let's start";
	while (true)
	{
		// sort nodes by number vms
		sort(nodeWrappers.begin(), nodeWrappers.end(), NodeWrapperComp);
		VmPoolNodeWrapper* nwSource = nodeWrappers.back();
		VmPoolNodeWrapper* nwDest = nodeWrappers.front();
		SYSLOGLOGGER(logDEBUG) << "source[n] " << nwSource->getName() << " " << nwSource->getNumberVms() <<
				     " dest[0] " << nwDest->getName() << " " << nwDest->getNumberVms();

		if (nwSource->getNumberVms() > (nwDest->getNumberVms() + tolerance))
		{
			SYSLOGLOGGER(logDEBUG) << "source:" << nwSource->getName() << " dest:" << nwDest->getName();
			//nSource->moveVmTo(nDest);
			nwSource->migrateFirstVm(nwDest, vt);
		}
		else
		{
			// the difference of the maximum vms in a node and the minimum vms in a node
			// is lower or equal than the tolerance value ->  leave the loop
			break;
		}
	}

	for (vector<VmPoolNodeWrapper*>::iterator itNodes = nodeWrappers.begin(); itNodes != nodeWrappers.end();)
	{
		VmPoolNodeWrapper* nodeWrapper = *itNodes++;
		SYSLOGLOGGER(logDEBUG) << "Nodes:" << nodeWrapper->getName();
		for (set<Vm*>::iterator itVms = nodeWrapper->getVms()->begin(); itVms != nodeWrapper->getVms()->end();)
		{
			Vm* vm = *itVms++;
			SYSLOGLOGGER(logDEBUG) << "Vm:" << vm->getName();
		}
	}
	return 0;
}

ostream& operator << (ostream& s, const EvenlyPolicy& ep)
{
    s << "EvenlyPolicy" << endl;
    s << "     minimalNumberOfVirtualMachines: " << ep.minimalNumberOfVirtualMachines << endl;
    s << "     maximalNumberOfVirtualMachines: " << ep.maximalNumberOfVirtualMachines << endl;
    s << "     preStartNumberOfVirtualMachines: " << ep.preStartNumberOfVirtualMachines << endl;
    s << "     tolerance: " << ep.tolerance << endl;
	return s;
}
