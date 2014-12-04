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
 * ldapTools.h
 *
 *  Created on: 16.05.2012
 *      Author: cwi
 */

#ifndef LDAPTOOLS_H_
#define LDAPTOOLS_H_

#include "LDAPConnection.h"
#include "LDAPConstraints.h"

class VmPool;
class Node;
class Vm;
class VmBackupConfiguration;
class VirtTools;
class NetworkRange;

class LdapTools {
private:
	LDAPConnection *lc;
	LDAPConstraints* constraints;
	LDAPControlSet* ctrlset;

public:
	LdapTools() : lc(NULL), constraints(NULL), ctrlset(NULL) {};

	virtual ~LdapTools() {
		unbind();
		delete ctrlset;
		delete constraints;
	};

	void bind();
	void unbind();

	void addEntry(const LDAPEntry* entry);
	void modifyEntry(const std::string& dn_, const LDAPModList* modification);
	void removeEntry(const std::string& dn_, bool recursive=false, bool keepStartDn=false, std::string prefix="");
	bool hasDn(const std::string& dn_);

	void readVmPools(const std::string& poolName, time_t actTime);
	VmPool* readVmPool(const std::string poolName, bool complete=false);
	Node* readNode(const std::string nodeName);
	void readVmsByPool(VmPool* vmPool, time_t actTime);
	Vm* readVm(const std::string vmName, bool complete=false);
	std::string readStoragePoolUri(const std::string& storagePoolName);
	std::string getNetworkRangeDn(const std::string& range);
	Vm* cloneVm(const Vm* vm, const Node* targetNode, VirtTools* vt, const std::string uuid);

	const std::string nextSpicePort(const Node* node);
	const std::string getFreeIp(const NetworkRange* range);

	const int getGlobalSetting(const std::string& setting) const;

	/* Backup */
	void readGlobalBackupConfiguration();

	/* Configuration */
	void readConfigurationSettings();
};

#endif /* LDAPTOOLS_H_ */
