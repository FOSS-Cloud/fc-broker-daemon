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
 * tPolicy.cpp
 *
 *  Created on: Jun 4, 2012
 *      Author: wre
 */

#include <iostream>
#include <cstdio>

#include "include/tPolicy.hpp"
#include "include/evenlyPolicy.hpp"
#include "include/node.hpp"
#include "include/vm.hpp"
#include "include/vmPool.hpp"

using namespace std;

const int tolerance = 3;

Vm* createVm(string name, string nodeName, string user)
{
	Vm* vm = new Vm("dn" + name, name);
	vm->setNodeName(nodeName);
	vm->setUser(user);
	std::cout << "createVm:" << vm << " Node:" << nodeName << std::endl;
	return vm;
}

Vm* TPolicy::createVm(VmPool* vmPool, string nodeName, string user)
{
	Vm* vm = vmFactory.createInstance();
	vm->setNodeName(nodeName);
	vm->setUser(user);
	vmPool->addVm(vm);
//	std::cout << "createVm:" << vm << " Node:" << nodeName << std::endl;
	return vm;
}

int TPolicy::checkResult(VmPool* vmPool, int nodeCount, int vmCount)
{
	int averageCount = 0;
	if (nodeCount > 0)
	{
		averageCount = vmCount / nodeCount;
	}
	int errorCount = 0;
	int maxVmCount = 0;
	int minVmCount = vmCount;
	for (map<string, Node*>::const_iterator it = vmPool->getNodes()->begin(); it != vmPool->getNodes()->end();)
	{
		Node* node = (*it++).second;
		if (node->getNumberVms() < minVmCount)
		{
			minVmCount = node->getNumberVms();
		}
		if (node->getNumberVms() > maxVmCount)
		{
			maxVmCount = node->getNumberVms();
		}

	}
	std::cout << "checkResult: nodeCount:" << nodeCount << " VmCount:" << vmCount <<
			" min:" << minVmCount << " max:" << maxVmCount << endl;
	if (tolerance < (maxVmCount - minVmCount))
	{
		for (map<string, Node*>::const_iterator it = vmPool->getNodes()->begin(); it != vmPool->getNodes()->end();)
		{
			Node* node = (*it++).second;
			std::cout << node->getName() << " " << node->getNumberVms() << endl;
		}
		errorCount = 0;
	}
	return 0;
}

void TPolicy::runTest()
{
	cout << "TPolicy::runTest()" << endl;
	for (int nodeCount = 0; nodeCount < 15; nodeCount++)
	{
		for (int vmCount = 0; vmCount < 45; vmCount++)
		{
			int nodeIndex = nodeCount / 3;
			for (int vmIndex = 0; vmIndex < vmCount; vmIndex++)
			{
				VmPool* vmPool = new VmPool("TestVmPool");
				EvenlyPolicy* policy = new EvenlyPolicy();
				policy->setTolerance(tolerance);
				vmPool->setPolicy(policy);
				for (int i = 0; i < nodeCount; i++)
				{
					vmPool->addNode(nodeFactory.createInstance());
				}
				for (int i = 0; i < vmCount; i++)
				{
					if (i == vmIndex && nodeCount > 1)
					{
						nodeIndex = (nodeIndex + 1) % nodeCount;
					}
					std::stringstream sb;
					sb << "Node";
					sb.width(3);
					sb.fill('0');
					sb << nodeIndex;
					createVm(vmPool, sb.str(), "");
				}
				vmPool->getPolicy()->checkPolicy(vmPool, &vmFactory);
				checkResult(vmPool, nodeCount, vmCount);
				delete vmPool;
				nodeFactory.reset();
				vmFactory.reset();
			}
		}
	}
}
