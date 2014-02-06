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
 * evenlyPolicy.hpp
 *
 *  Created on: 18.05.2012
 *      Author: cwi
 */

#ifndef EVENLYPOLICY_HPP_
#define EVENLYPOLICY_HPP_

#include <vector>

#include "basePolicy.hpp"
#include "vm.hpp"

class VmPoolNodeWrapper;
class VirtTools;

class EvenlyPolicy : public BasePolicy {
protected:
	std::vector<VmPoolNodeWrapper*> nodeWrappers;
	int numberVms;
	int numberPrestartedVms;
	int numberVmsToStart;
	int maxVmPerNode;

	int minimalNumberOfVirtualMachines;
	int maximalNumberOfVirtualMachines;
	int preStartNumberOfVirtualMachines;
	int tolerance;

public:
	EvenlyPolicy()
	: numberVms(0)
	, numberPrestartedVms(0)
	, numberVmsToStart(0)
	, maxVmPerNode(0)
	, minimalNumberOfVirtualMachines(-1)
	, maximalNumberOfVirtualMachines(-1)
	, preStartNumberOfVirtualMachines(-1)
	, tolerance(1)
	{};
	virtual ~EvenlyPolicy() {};

	virtual int checkPolicy(VmPool* vmPool, VmFactory* vmFactory, VirtTools* vt);
	virtual bool preStart(VmPool* vmPool, VmFactory* vmFactory);

	int checkPreStartPolicyTest(VmPool* vmPool, VmFactory* vmFactory) const;
	int checkEvenlyPolicyTest(VmPool* vmPool, VirtTools* vt) const;

	void setMinimalNumberOfVirtualMachines(int minimalNumberOfVirtualMachines_)
	{
		minimalNumberOfVirtualMachines = minimalNumberOfVirtualMachines_;
	}
	void setMaximalNumberOfVirtualMachines(int maximalNumberOfVirtualMachines_)
	{
		maximalNumberOfVirtualMachines = maximalNumberOfVirtualMachines_;
	}
	void setPreStartNumberOfVirtualMachines(int preStartNumberOfVirtualMachines_)
	{
		preStartNumberOfVirtualMachines = preStartNumberOfVirtualMachines_;
	}
	void setTolerance(int tolerance_)
	{
		tolerance = tolerance_;
		if (tolerance < 1)
		{
			tolerance = 1;
		}
	}
	int getMinimalNumberOfVirtualMachines()
	{
		return minimalNumberOfVirtualMachines;
	}
	int getMaximalNumberOfVirtualMachines()
	{
		return maximalNumberOfVirtualMachines;
	}
	int getPreStartNumberOfVirtualMachines()
	{
		return preStartNumberOfVirtualMachines;
	}
	int getTolerance()
	{
		return tolerance;
	}

	friend std::ostream& operator <<(std::ostream& s, const EvenlyPolicy& ep);
};

#endif /* EVENLYPOLICY_HPP_ */
