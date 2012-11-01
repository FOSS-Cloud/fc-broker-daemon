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
 * tLdap.cpp
 *
 *  Created on: Jun 24, 2012
 *      Author: cwi
 */

#include <iostream>
#include <cstdio>

#include "LDAPModList.h"

#include "include/config.hpp"
#include "include/tLdap.hpp"
#include "include/ldapTools.hpp"

using namespace std;

void TLdap::init()
{
	Config::filename = "/etc/foss-cloud/config.xml";
	Config* config = Config::getInstance();
	const char * dn = config->getLdapBindDn();
	cout << "getLdapBindDn " << dn << endl;
}

void TLdap::runTest()
{
	cout << "TLdap::runTest()" << endl;

	init();

	LdapTools* lt = new LdapTools();
	lt->bind();

	try {
		string dn = "uid=4000007,ou=people,dc=foss-cloud,dc=org";
		cout << dn << "; " << "sstGender=f" << endl;
		LDAPModList* modlist = new LDAPModList();
		cout << "modlist";
		LDAPAttribute attribute = LDAPAttribute("sstGender", "f");
		cout << "; attribute";
		LDAPModification modification = LDAPModification(attribute, LDAPModification::OP_REPLACE);
		cout << "; modification";
		modlist->addModification(modification);
		cout << "; addModification";
		lt->modifyEntry(dn, modlist);
		cout << "; modify" << endl;
	}
	catch (LDAPException &e) {
		std::cout << "-------------- caught LDAPException ---------" << std::endl;
		std::cout << e << std::endl;
	}
}

void TLdap::runTest2()
{
	cout << "TLdap::runTest2()" << endl;

	init();

	LdapTools* lt = new LdapTools();
	lt->bind();

	try {
		string dn = "sstVirtualMachine=4fe820ca-2467-405d-8418-a39b14254582,ou=virtual machines,ou=virtualization,ou=services,dc=foss-cloud,dc=org";
		string attr = "sstNode";
		string value = "foss-cloud-node-02_CWI";
		cout << dn << "; " << attr << "=" << value << endl;
		LDAPModList* modlist = new LDAPModList();
		cout << "modlist";
		LDAPAttribute attribute = LDAPAttribute(attr, value);
		cout << "; attribute";
		LDAPModification modification = LDAPModification(attribute, LDAPModification::OP_REPLACE);
		cout << "; modification";
		modlist->addModification(modification);
		cout << "; addModification";
		lt->modifyEntry(dn, modlist);
		cout << "; modify" << endl;
	}
	catch (LDAPException &e) {
		std::cout << "-------------- caught LDAPException ---------" << std::endl;
		std::cout << e << std::endl;
	}
}
