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
 * tPolicy.hpp
 *
 *  Created on: 11.05.2012
 *      Author: wre
 */

#include "factory.hpp"
#include "node.hpp"
#include "vm.hpp"
#include "vmPool.hpp"

class TPolicy
{
	Factory<Node> nodeFactory;
	Factory<Vm> vmFactory;
public:
	TPolicy()
	: 	nodeFactory("Node"),
		vmFactory("Vm")
	{}

	Vm* createVm(VmPool* vmPool, std::string nodeName, std::string user);
	int checkResult(VmPool* vmPool, int nodeCount, int vmCount);
	void runTest();
};
