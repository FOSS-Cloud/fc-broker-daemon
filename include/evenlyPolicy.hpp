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
 * evenlyPolicy.hpp
 *
 *  Created on: 18.05.2012
 *      Author: cwi
 */

#ifndef EVENLYPOLICY_HPP_
#define EVENLYPOLICY_HPP_

#include "basePolicy.hpp"
#include "vm.hpp"

class VirtTools;

class EvenlyPolicy : public BasePolicy {
private:
	int minimalNumberOfVirtualMachines;
	int maximalNumberOfVirtualMachines;
	int preStartNumberOfVirtualMachines;
	int tolerance;

public:
	EvenlyPolicy()
	: minimalNumberOfVirtualMachines(-1)
	, maximalNumberOfVirtualMachines(-1)
	, preStartNumberOfVirtualMachines(-1)
	, tolerance(1)
	{};
	virtual ~EvenlyPolicy() {};

	virtual int checkPolicy(VmPool* vmPool, VmFactory* vmFactory, VirtTools* vt) const;
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
	friend std::ostream& operator <<(std::ostream& s, const EvenlyPolicy& ep);

};

#endif /* EVENLYPOLICY_HPP_ */
