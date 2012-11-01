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
 * basePolicy.hpp
 *
 *  Created on: 18.05.2012
 *      Author: cwi
 */

#ifndef BASEPOLICY_HPP_
#define BASEPOLICY_HPP_

class VmPool;
class VmFactory;
class VirtTools;

class BasePolicy {
public:
	BasePolicy() {};
	virtual ~BasePolicy() {};
	virtual int checkPolicy(VmPool* vmPool, VmFactory* vmFactory, VirtTools* vt) const {return 0;};
};

#endif /* BASEPOLICY_HPP_ */
