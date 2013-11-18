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
 * vmFactory.h
 *
 *  Created on: 28.06.2012
 *      Author: cwi
 */

#ifndef VMFACTORY_H_
#define VMFACTORY_H_

class LdapTools;
class VirtTools;
class Vm;

class VmFactory {
private:
	LdapTools* lt;
	VirtTools* vt;
public:
	VmFactory(LdapTools* lt_, VirtTools* vt_) : lt(lt_), vt(vt_) {};
	VmFactory() : lt(NULL), vt(NULL) {};
	virtual ~VmFactory() {};

	Vm* createInstance(const Vm* goldenImage, const Node* node) const;
};

#endif /* VMFACTORY_H_ */
