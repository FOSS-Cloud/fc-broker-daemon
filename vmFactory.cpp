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
 * vmFactory.cpp
 *
 *  Created on: 28.06.2012
 *      Author: cwi
 */

#include "include/ldapTools.hpp"
#include "include/virtTools.hpp"
#include "include/node.hpp"
#include "include/vmFactory.hpp"
#include "include/logger.hpp"

using namespace std;

Vm* VmFactory::createInstance(const Vm* goldenImage, const Node* node) const {
	Vm* retval = NULL;

	SYSLOGLOGGER(logDEBUG) << "createInstance on " << node->getName();

	retval = lt->cloneVm(goldenImage, node, vt, vt->generateUUID());
	try {
		vt->startVm(retval);
		LDAPModList* modlist = new LDAPModList();
		LDAPAttribute attr = LDAPAttribute("sstStatus", "running");
		LDAPModification modification = LDAPModification(attr, LDAPModification::OP_REPLACE);
		modlist->addModification(modification);
		SYSLOGLOGGER(logDEBUG) << (retval->getName()) << ": set sstStatus to running";
		lt->modifyEntry(retval->getDn(), modlist);
		delete modlist;
	}
	catch(VirtException &e) {
		SYSLOGLOGGER(logINFO) << "createInstance -------------- caught VirtException ---------";
		SYSLOGLOGGER(logINFO) << e;
		retval->remove();
		delete retval;
		retval = NULL;
		throw e;
	}
	catch(LDAPException &e) {
		SYSLOGLOGGER(logINFO) << "createInstance -------------- caught LDAPException ---------";
		SYSLOGLOGGER(logINFO) << e;
	}
	return retval;
}

