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
 * ldapTools.cpp
 *
 *  Created on: 16.05.2012
 *      Author: cwi
 */

#include "cstdlib"

#include "LDAPConnection.h"
#include "LDAPConstraints.h"
#include "LDAPAttribute.h"
#include "LDAPAttributeList.h"
#include "LDAPEntry.h"
#include "LDAPException.h"
#include "LDAPModification.h"

#include "include/ldapTools.hpp"
#include "include/virtTools.hpp"
#include "include/config.hpp"
#include "include/vmPool.hpp"
#include "include/vm.hpp"
#include "include/node.hpp"
#include "include/evenlyPolicyInterval.hpp"
#include "include/logger.hpp"

using namespace std;

void LdapTools::bind() {
	if (NULL != constraints) {
		delete constraints;
	}
	constraints = new LDAPConstraints();
	if (NULL != ctrlset) {
		delete ctrlset;
	}
	ctrlset = new LDAPControlSet();
	ctrlset->add(LDAPCtrl(LDAP_CONTROL_MANAGEDSAIT));
	constraints->setServerControls(ctrlset);
	lc = new LDAPConnection(Config::getInstance()->getLdapUri(), /* just to add something: */ 2701, constraints);
	SYSLOGLOGGER(logINFO) << "----------------------doing bind.... ";

	lc->bind(Config::getInstance()->getLdapBindDn(), Config::getInstance()->getLdapBindPwd(), constraints);
	SYSLOGLOGGER(logINFO) << "                                     " << lc->getHost();
}

void LdapTools::unbind() {
	if (NULL != lc) {
		try {
			lc->unbind();
		}
		catch (LDAPException &e) {}
		delete lc;
		lc = NULL;
	}
}

void LdapTools::addEntry(const LDAPEntry* entry) {
	lc->add(entry);
}

void LdapTools::modifyEntry(const std::string& dn_, const LDAPModList* modification) {
	lc->modify(dn_, modification);
}

void LdapTools::removeEntry(const std::string& dn_, bool recursive, bool keepStart, std::string prefix) {
	SYSLOGLOGGER(logDEBUG) << prefix << "removeEntry " << dn_;
	if (!recursive) {
		lc->del(dn_);
	}
	else {
		LDAPSearchResults* entries = lc->search(dn_, LDAPConnection::SEARCH_ONE);
		if (entries != 0) {
			LDAPEntry* entry = entries->getNext();
			while (entry != 0) {
				//if (0 != dn_.compare(entry->getDN())) {
				removeEntry(entry->getDN(), recursive, false, prefix + "   ");
				//}
				delete entry;
				entry = entries->getNext();
			}
			if (!keepStart) {
				SYSLOGLOGGER(logDEBUG) << prefix << "   remove " << dn_;
				lc->del(dn_);
			}
		}
	}
}

bool LdapTools::hasDn(const std::string& dn_) {
	try {
		LDAPSearchResults* entries = lc->search(dn_, LDAPConnection::SEARCH_ONE);
		return entries != 0;
	}
	catch (LDAPException &e) {
		if (32 == e.getResultCode()) {
			// No such object
			SYSLOGLOGGER(logINFO) << "hasDn: No such object";
			return false;
		}
	}
	return false;
}

void LdapTools::readVmPools(const std::string& poolName, time_t actTime) {
	string base("ou=virtual machine pools,ou=virtualization,ou=services,");
	base.append(Config::getInstance()->getLdapBaseDn());
	LDAPSearchResults* entries = lc->search(base.c_str(), LDAPConnection::SEARCH_ONE);
	if (entries != 0) {
		LDAPEntry* entry = entries->getNext();
		while (entry != 0) {
//			SYSLOGLOGGER(logINFO) << "readVmPools dn: " << entry->getDN();
			VmPool* vmPool = readVmPool(entry->getDN(), true);
			SYSLOGLOGGER(logINFO) << "VmPool.type: " << (vmPool->getType());
			// Todo: check if there min. 2 nodes in this pool
			if (0 == poolName.length() || 0 == poolName.compare(vmPool->getName())) {
				if (vmPool->isDynamicType()) {
					if (vmPool->hasActiveGoldenImage()) {
						readVmsByPool(vmPool, actTime);
						if (NULL == Config::getInstance()->getVmPoolByName(vmPool->getName())) {
							SYSLOGLOGGER(logINFO) << "  pool added!";
							Config::getInstance()->addVmPool(vmPool);
						}
					}
					else {
						SYSLOGLOGGER(logINFO) << "  !! not used (VmPool has no active golden image)";
						delete vmPool;
					}
					if (vmPool->hasShutdownConfiguration() && vmPool->isShutdownTime(actTime)) {
						Config::getInstance()->addShutdownVmPool(vmPool);
					}
				}
				else if (vmPool->isStaticType() || vmPool->isTemplateType()) {
					SYSLOGLOGGER(logINFO) << "readVmsByPool";
					// readVmsByPool decides what to do
					readVmsByPool(vmPool, actTime);
				}
				else {
					SYSLOGLOGGER(logINFO) << "  !! not used (unknown type)";
					delete vmPool;
				}
			}
			else {
				SYSLOGLOGGER(logINFO) << "  !! not used (wrong pool)";
				delete vmPool;
			}
			delete entry;
			entry = entries->getNext();
		}
	}
}

VmPool* LdapTools::readVmPool(const string poolName, bool complete) {
	VmPool* retval = NULL;
	string base;

	if (!complete) {
		base = string("sstVirtualMachinePool=");
		base.append(poolName).append(",ou=virtual machine pools,ou=virtualization,ou=services,").append(
				Config::getInstance()->getLdapBaseDn());
	}
	else {
		base = string(poolName);
	}
	SYSLOGLOGGER(logINFO) << "readVmPool " << poolName;
	LDAPSearchResults* entries = lc->search(base, LDAPConnection::SEARCH_SUB);
	if (entries != 0) {
		SYSLOGLOGGER(logINFO) << "   found!";
		LDAPEntry* entry = entries->getNext();
		if (entry != 0) {
			BasePolicy* policy = NULL;
			const LDAPAttribute* attr = entry->getAttributeByName("sstVirtualMachinePool");
			StringList values = attr->getValues();
			StringList::const_iterator it2 = values.begin();
			string poolName = *it2;
			SYSLOGLOGGER(logINFO) << "readVmPool " << poolName;
			attr = entry->getAttributeByName("sstVirtualMachinePoolType");
			values = attr->getValues();
			it2 = values.begin();
			string poolType = *it2;
			SYSLOGLOGGER(logINFO) << "   type: " << poolType;
			if (0 == poolType.compare("dynamic")) {
				if (NULL != entry->getAttributeByName("sstBrokerPreStartInterval")) {
					policy = new EvenlyPolicyInterval();
					SYSLOGLOGGER(logDEBUG) << "EvenlyPolicyInterval ";
				}
				else {
					policy = new EvenlyPolicy();
					SYSLOGLOGGER(logDEBUG) << "EvenlyPolicy ";
				}
			}
			const VmPool* pool = Config::getInstance()->getVmPoolByName(poolName);
			SYSLOGLOGGER(logINFO) << "   found! " << pool;
			if (NULL == pool) {
				retval = new VmPool(entry->getDN(), this);
			}
			else {
				retval = const_cast<VmPool*>(pool);
			}
			retval->setPolicy(policy);
			//SYSLOGLOGGER(logINFO) << "retval (VmPool) " << retval;
		}
		while (entry != 0) {
//			SYSLOGLOGGER(logINFO) << "dn: " << entry->getDN();
			const LDAPAttributeList* attrs = entry->getAttributes();
			LDAPAttributeList::const_iterator it = attrs->begin();
			for (; it != attrs->end(); it++) {
				LDAPAttribute attr = *it;
//				SYSLOGLOGGER(logINFO) << attr.getName() << "(";
				StringList values = attr.getValues();
				StringList::const_iterator it2 = values.begin();
				string value = *it2;
//				for (; it2 != values.end(); it2++) {
//					SYSLOGLOGGER(logINFO) << *it2 << "; ";
//				}
//				SYSLOGLOGGER(logINFO) << ")" << std::endl;
				retval->addAttribute(entry->getDN(), attr.getName(), value);
			}
			delete entry;
			entry = entries->getNext();
		}
	}
	if (!retval->hasOwnBackupConfiguration()) {
		retval->setBackupConfiguration(Config::getInstance()->getGlobalBackupConfiguration());
		SYSLOGLOGGER(logINFO) << "  use global backupconf for vmPool " << retval->getName() << "!";
		SYSLOGLOGGER(logINFO) << "  " << *(Config::getInstance()->getGlobalBackupConfiguration());
	}
	retval->checkAllowUSB();
	retval->checkAllowSound();
	SYSLOGLOGGER(logINFO) << "retval (VmPool) " << *retval;
	return retval;
}

Node* LdapTools::readNode(const string nodeName) {
	Node* retval = NULL;
	string base("sstNode=");
	base.append(nodeName).append(",ou=nodes,ou=virtualization,ou=services,").append(
			Config::getInstance()->getLdapBaseDn());
	SYSLOGLOGGER(logDEBUG) << "readNode " << base;
	LDAPSearchResults* entries = lc->search(base, LDAPConnection::SEARCH_SUB);
	if (entries != 0) {
		LDAPEntry* entry = entries->getNext();
		if (entry != 0) {
			retval = new Node(entry->getDN(), this);
		}
		while (entry != 0) {
//			SYSLOGLOGGER(logINFO) << "dn: " << entry->getDN();
			const LDAPAttributeList* attrs = entry->getAttributes();
			LDAPAttributeList::const_iterator it = attrs->begin();
			for (; it != attrs->end(); it++) {
				LDAPAttribute attr = *it;
//				SYSLOGLOGGER(logINFO) << attr.getName() << "(";
//				SYSLOGLOGGER(logINFO) << attr.getNumValues() << "): ";
				StringList values = attr.getValues();
				StringList::const_iterator it2 = values.begin();
				string value = *it2;
//				for (; it2 != values.end(); it2++) {
//
//					SYSLOGLOGGER(logINFO) << *it2 << "; ";
//				}
//				SYSLOGLOGGER(logINFO) << std::endl;
				retval->addAttribute(entry->getDN(), attr.getName(), value);
			}
			delete entry;
			entry = entries->getNext();
		}
		if (NULL != retval) {
			NodeType* type = retval->getType(string("VM-Node"));
			string nodestate = type->getState();
			retval->setMaintenance(0 == nodestate.compare("maintenance"));
		}
	}
	return retval;
}

void LdapTools::readVmsByPool(VmPool* vmPool, time_t actTime) {
	string base("ou=virtual machines,ou=virtualization,ou=services,");
	base.append(Config::getInstance()->getLdapBaseDn());
	string filter("(&(objectClass=*)(sstVirtualMachinePool=");
	filter.append(vmPool->getName()).append("))");
	LDAPSearchResults* entries = lc->search(base, LDAPConnection::SEARCH_ONE, filter);
	if (entries != 0) {
		LDAPEntry* entry = entries->getNext();
		while (entry != 0) {
			SYSLOGLOGGER(logINFO) << "readVms dn: " << entry->getDN();
			Vm* vm = readVm(entry->getDN(), true);
			if (NULL != vm) {
				if (vm->isDynVm()) {
					if (vm->isGoldenImage()) {
						SYSLOGLOGGER(logINFO) << "  !! not used for policy; is Golden-Image!";
						//delete vm;

						if (!vm->hasOwnBackupConfiguration()) {
							vm->setBackupConfiguration(vmPool->getBackupConfiguration());
							SYSLOGLOGGER(logINFO) << "  use backupconf from vmPool " << vmPool->getName() << "!";
						}
						if (vm->isBackupNeeded()) {
							Config::getInstance()->handleVmForBackup(vm, actTime);
						}
					}
					else {
						vmPool->addVm(vm);
						Config::getInstance()->addVm(vm);
					}
				}
				else {
					if (!vm->hasOwnBackupConfiguration()) {
						vm->setBackupConfiguration(vmPool->getBackupConfiguration());
						SYSLOGLOGGER(logINFO) << "  use backupconf from vmPool " << vmPool->getName() << "!";
					}
					SYSLOGLOGGER(logINFO) << "Vm: " << *vm;
					if (vm->isBackupNeeded()) {
						Config::getInstance()->handleVmForBackup(vm, actTime);
					}
				}
			}

			delete entry;
			entry = entries->getNext();
		}
	}
	//SYSLOGLOGGER(logINFO) << "readVmsByPool finished";
}

Vm* LdapTools::readVm(const string vmName, bool complete) {
	Vm* retval = NULL;
	string base;

	if (!complete) {
		base = string("sstVirtualMachine=");
		base.append(vmName).append(",ou=virtual machines,ou=virtualization,ou=services,").append(
				Config::getInstance()->getLdapBaseDn());
	}
	else {
		base = string(vmName);
	}
	SYSLOGLOGGER(logINFO) << "readVm " << base;
	LDAPSearchResults* entries = NULL;
	try {
		entries = lc->search(base, LDAPConnection::SEARCH_SUB);
	}
	catch (LDAPException &e) {
		SYSLOGLOGGER(logERROR) << "readVm -------------- caught LDAPException ---------";
		SYSLOGLOGGER(logERROR) << e;
		if (32 != e.getResultCode()) {
			// No Such Object
			throw;
		}
		else {
			entries = NULL;
		}
	}
	if (entries != 0) {
		LDAPEntry* entry = entries->getNext();
		if (entry != 0) {
			retval = new Vm(entry->getDN(), this);
		}
		while (entry != 0) {
//			SYSLOGLOGGER(logINFO) << "dn: " << entry->getDN();
			const LDAPAttributeList* attrs = entry->getAttributes();
			LDAPAttributeList::const_iterator it = attrs->begin();
			for (; it != attrs->end(); it++) {
				LDAPAttribute attr = *it;
//				SYSLOGLOGGER(logINFO) << attr.getName() << "(" << attr.getNumValues() << "): ";
				StringList values = attr.getValues();
				StringList::const_iterator it2 = values.begin();
				string value = *it2;
//				for (; it2 != values.end(); it2++) {
//					SYSLOGLOGGER(logINFO) << *it2 << "; ";
//				}
				if (0 == attr.getName().compare("sstFeature")) {
					retval->setFeatures(values);
				}
				else {
					retval->addAttribute(entry->getDN(), attr.getName(), value);
				}
			}
			delete entry;
			entry = entries->getNext();
		}
		retval->checkAllowUSB();
		retval->checkAllowSound();
	}
	//SYSLOGLOGGER(logINFO) << "readVm finished!";
	return retval;
}

string LdapTools::readStoragePoolUri(const string& storagePoolName) {
	string retval = "";
	string base("sstStoragePool=");
	base.append(storagePoolName).append(",ou=storage pools,ou=virtualization,ou=services,").append(
			Config::getInstance()->getLdapBaseDn());
	SYSLOGLOGGER(logDEBUG) << "readStoragePool " << base;
	LDAPSearchResults* entries = lc->search(base, LDAPConnection::SEARCH_SUB);
	if (entries != 0) {
		LDAPEntry* entry = entries->getNext();
		if (entry != 0) {
//			SYSLOGLOGGER(logINFO) << "dn: " << entry->getDN() << endl;
			const LDAPAttributeList* attrs = entry->getAttributes();
			LDAPAttributeList::const_iterator it = attrs->begin();
			for (; it != attrs->end(); it++) {
				LDAPAttribute attr = *it;
//				SYSLOGLOGGER(logINFO) << attr.getName() << "(";
//				SYSLOGLOGGER(logINFO) << attr.getNumValues() << "): ";
				if (0 == attr.getName().compare("sstStoragePoolURI")) {
					StringList values = attr.getValues();
					StringList::const_iterator it2 = values.begin();
					if (it2 != values.end()) {
						retval = *it2;
						break;
					}
				}
			}
			delete entry;
		}
	}
	return retval;
}

string LdapTools::getNetworkRangeDn(const string& range) {
	string retval = "";
	string base("ou=dhcp,ou=networks,ou=virtualization,ou=services,");
	base.append(Config::getInstance()->getLdapBaseDn());
	SYSLOGLOGGER(logDEBUG) << "find NetworkRange " << range;
	string filter = "(&(objectClass=sstVirtualizationNetworkRange)(cn=";
	filter.append(range).append("))");
	//SYSLOGLOGGER(logINFO) << "dhcp base: " << base << "; filter " << filter;
	StringList attrs = StringList();
	attrs.add("cn");
	LDAPSearchResults* entries = lc->search(base, LDAPConnection::SEARCH_SUB, filter, attrs);
	if (entries != 0) {
		LDAPEntry* entry = entries->getNext();
		if (entry != 0) {
			retval = string(entry->getDN());
		}
		delete entry;
	}
	//SYSLOGLOGGER(logINFO) << "dn: " << retval;

	return retval;
}

Vm* LdapTools::cloneVm(const Vm* vm, const Node* targetNode, VirtTools* vt, string newUuid) {
	Vm* retval = NULL;
	size_t pos;
	const VmPool* vmPool = vm->getVmPool();

	SYSLOGLOGGER(logDEBUG) << "cloneVm DN: " << (vm->getDn());
	LDAPSearchResults* entries = lc->search(vm->getDn(), LDAPConnection::SEARCH_SUB);
	if (entries != 0) {
		string newVmDn;

		LDAPEntry* entry = entries->getNext();
		const string oldVmDn = entry->getDN();
		string uuid;
		vm->getDnPart(oldVmDn, "sstVirtualMachine", uuid);
		SYSLOGLOGGER(logDEBUG) << "old DN: " << oldVmDn << "; uuid: " << uuid;
		string oldVm = "sstVirtualMachine=";
		oldVm.append(uuid);

		string newVm = "sstVirtualMachine=" + newUuid;
		pos = oldVmDn.find(oldVm);
		newVmDn = newVm;
		newVmDn.append(oldVmDn.substr(oldVm.length()));
		SYSLOGLOGGER(logDEBUG) << "new DN: " << newVmDn;

		string firstMac = "";
		bool diskSet = false;

		while (entry != 0) {
			string dn = entry->getDN();
			pos = dn.find(oldVm);
			string newDn = dn.substr(0, pos);
			newDn.append(newVm).append(dn.substr(pos + oldVm.length()));
			//SYSLOGLOGGER(logDEBUG) << "oldDn: " << dn;
			//SYSLOGLOGGER(logDEBUG) << "newDn: " << newDn;

			LDAPEntry* newEntry = new LDAPEntry(newDn, entry->getAttributes());
			if (0 == newDn.find("sstVirtualMachine")) {
				// DN starts with sstVirtualMachine
				newEntry->delAttribute("sstVirtualMachine");
				const LDAPAttribute* attribute = entry->getAttributeByName("sstDisplayName");
				StringList values = attribute->getValues();
				StringList::const_iterator it = values.begin();
				string displayName = *it;
				displayName.append(" clone");
				newEntry->replaceAttribute(LDAPAttribute("sstDisplayName", displayName));
				newEntry->replaceAttribute(LDAPAttribute("sstNode", targetNode->getName()));
				newEntry->replaceAttribute(LDAPAttribute("sstVirtualMachineType", "dynamic"));
				newEntry->replaceAttribute(LDAPAttribute("sstVirtualMachineSubType", "Desktop"));
				newEntry->replaceAttribute(LDAPAttribute("sstOsBootDevice", "hd"));
				newEntry->replaceAttribute(LDAPAttribute("sstSpicePort", nextSpicePort(targetNode)));
				newEntry->replaceAttribute(LDAPAttribute("sstSpicePassword", newUuid));
			}
			else if (0 == newDn.find("sstDisk") && !diskSet) {
				// DN starts with sstDisk
				const LDAPAttribute* attribute = entry->getAttributeByName("sstDevice");
				StringList values = attribute->getValues();
				StringList::const_iterator it = values.begin();
				string value = *it;
				if (0 == value.compare("disk")) {
					const string volumeName = vt->generateUUID();
					string sourceFile = vmPool->getStoragePoolDir();
					sourceFile.append("/").append(volumeName).append(".qcow2");
					SYSLOGLOGGER(logINFO) << "volumeName: " << volumeName;
					SYSLOGLOGGER(logINFO) << "sourceFile: " << sourceFile;
					try {
						vt->createBackingStoreVolumeFile(vm, vmPool->getStoragePoolName(), volumeName);
					}
					catch (VirtException& e) {
						SYSLOGLOGGER(logINFO) << "-------------- caught Exception ---------";
						SYSLOGLOGGER(logINFO) << e;
						lc->del(newDn);
						delete entry;
						return NULL;
					}

					newEntry->replaceAttribute(LDAPAttribute("sstVolumeName", volumeName));
					newEntry->replaceAttribute(LDAPAttribute("sstSourceFile", sourceFile));
					diskSet = true;
				}
			}
			else if (0 == newDn.find("sstInterface") && 0 == firstMac.size()) {
				// DN start with sstInterface
				firstMac = vt->generateMacAddress();
				newEntry->replaceAttribute(LDAPAttribute("sstMacAddress", firstMac));
			}
			lc->add(newEntry);

			delete entry;
			entry = entries->getNext();
		}
		string peopleDn = "ou=people,";
		peopleDn.append(newVmDn);

		LDAPEntry* peopleEntry = new LDAPEntry(peopleDn);
		StringList values;
		values.add("top");
		values.add("organizationalUnit");
		values.add("sstRelationship");

		peopleEntry->addAttribute(LDAPAttribute("objectClass", values));
		peopleEntry->addAttribute(LDAPAttribute("ou", "people"));
		peopleEntry->addAttribute(LDAPAttribute("description", "This is the assigned people subtree."));
		peopleEntry->addAttribute(LDAPAttribute("sstBelongsToCustomerUID", vm->getCustomerUID()));
		peopleEntry->addAttribute(LDAPAttribute("sstBelongsToResellerUID", vm->getResellerUID()));
		lc->add(peopleEntry);
		delete peopleEntry;

		const NetworkRange* range = vmPool->getRange();
		string base = "ou=dhcp,ou=networks,ou=virtualization,ou=services,";
		base.append(Config::getInstance()->getLdapBaseDn());
		string filter = "(&(objectClass=sstVirtualizationNetworkRange)(cn=";
		filter.append(range->getRange()).append("))");
		SYSLOGLOGGER(logDEBUG) << "dhcp base: " << base << "; filter " << filter;
		StringList attrs = StringList();
		attrs.add("cn");
		LDAPSearchResults* entries2 = lc->search(base, LDAPConnection::SEARCH_SUB, filter, attrs);
		if (NULL != entries2) {
			LDAPEntry* entry2 = entries2->getNext();
			if (NULL != entry2) {
				string dn = "cn=";
				dn.append(newUuid).append(",ou=virtual machines,");
				string entryDn = entry2->getDN();
				delete entry2;
				SYSLOGLOGGER(logDEBUG) << "rangeDN: " << entryDn;
				pos = entryDn.find("ou=ranges,");
				dn.append(entryDn.substr(pos + 10));
				SYSLOGLOGGER(logDEBUG) << "dhcp dn:" << dn;
				LDAPEntry* dhcpEntry = new LDAPEntry(dn);
				StringList vals;
				vals.add("top");
				vals.add("dhcpHost");
				vals.add("sstVirtualizationNetwork");

				dhcpEntry->addAttribute(LDAPAttribute("objectClass", vals));
				dhcpEntry->addAttribute(LDAPAttribute("cn", newUuid));
				dhcpEntry->addAttribute(LDAPAttribute("sstBelongsToCustomerUID", vm->getCustomerUID()));
				dhcpEntry->addAttribute(LDAPAttribute("sstBelongsToResellerUID", vm->getResellerUID()));
				dhcpEntry->addAttribute(LDAPAttribute("dhcpHWAddress", "ethernet " + firstMac));
				dhcpEntry->addAttribute(LDAPAttribute("dhcpStatements", "fixed-address " + getFreeIp(range)));
				lc->add(dhcpEntry);
				delete dhcpEntry;
			}
		}

		retval = readVm(newVmDn, true);
	}

	return retval;
}

const string LdapTools::nextSpicePort(const Node* node) {
	int port = 0;
	int portMin = Config::getInstance()->getSpicePortMin();
	int portMax = Config::getInstance()->getSpicePortMax();
	int size = portMax - portMin + 1;
	//bool* portsUsed = new bool[size];
	bool* portsUsed = (bool *) malloc(size * sizeof(bool));
	for (int i = 0; i < size; i++) {
		portsUsed[i] = false;
	}
	string base("ou=virtual machines,ou=virtualization,ou=services,");
	base.append(Config::getInstance()->getLdapBaseDn());
//	string filter = "(&(objectClass=sstSpice))";
	string filter = "(&(objectClass=sstSpice)(sstNode=";
	filter.append(node->getName()).append("))");
	StringList attrs = StringList();
	attrs.add("sstVirtualMachine");
	attrs.add("sstSpicePort");
	LDAPSearchResults* entries = lc->search(base, LDAPConnection::SEARCH_SUB, filter, attrs);
	LDAPEntry* entry = entries->getNext();
	while (entry != 0) {
		string vmName = "";
		const LDAPAttribute* attribute = entry->getAttributeByName("sstVirtualMachine");
		const StringList values = attribute->getValues();
		StringList::const_iterator it = values.begin();
		if (it != values.end()) {
			vmName = it->c_str();
		}
		const LDAPAttribute* attribute2 = entry->getAttributeByName("sstSpicePort");
		const StringList values2 = attribute2->getValues();
		StringList::const_iterator it2 = values2.begin();
		if (it2 != values2.end()) {
			port = atoi(it2->c_str());
			SYSLOGLOGGER(logDEBUG) << "  " << port << " in use " << port - portMin << " (" << vmName << ")";
			portsUsed[port - portMin] = true;
		}
		delete entry;
		entry = entries->getNext();
	}

	filter = "(&(objectClass=sstSpice)(sstMigrationNode=";
	filter.append(node->getName()).append("))");
	attrs = StringList();
	attrs.add("sstMigrationSpicePort");
	entries = lc->search(base, LDAPConnection::SEARCH_SUB, filter, attrs);
	entry = entries->getNext();
	while (entry != 0) {
		const LDAPAttribute* attribute = entry->getAttributeByName("sstMigrationSpicePort");
		const StringList values = attribute->getValues();
		StringList::const_iterator it = values.begin();
		if (it != values.end()) {
			port = atoi(it->c_str());
			SYSLOGLOGGER(logDEBUG) << "M " << port << " in use " << port - portMin;
			portsUsed[port - portMin] = true;
		}
		delete entry;
		entry = entries->getNext();
	}

	port = 0;
	for (int i = 0; i < size; i++) {
		if (!portsUsed[i]) {
			port = portMin + i;
			break;
		}
	}

	//delete[] portsUsed;
	free(portsUsed);

	SYSLOGLOGGER(logDEBUG) << "nextSpicePort: " << base << "; " << filter << "; port: " << port;
	char buffer[10];
	sprintf(buffer, "%d", port);
	return string(buffer);
}

void LdapTools::readGlobalBackupConfiguration() {
	VmBackupConfiguration* config = Config::getInstance()->getGlobalBackupConfiguration();
	string base("ou=backup,ou=configuration,ou=virtualization,ou=services,");
	base.append(Config::getInstance()->getLdapBaseDn());
	SYSLOGLOGGER(logDEBUG) << "readGlobalBackupConfiguration " << base;
	LDAPSearchResults* entries = lc->search(base, LDAPConnection::SEARCH_SUB);
	if (entries != 0) {
		LDAPEntry* entry = entries->getNext();
		while (entry != 0) {
//			SYSLOGLOGGER(logINFO) << "dn: " << entry->getDN();
			const LDAPAttributeList* attrs = entry->getAttributes();
			LDAPAttributeList::const_iterator it = attrs->begin();
			for (; it != attrs->end(); it++) {
				LDAPAttribute attr = *it;
//				SYSLOGLOGGER(logINFO) << attr.getName() << "(" << attr.getNumValues() << "): ";
				StringList values = attr.getValues();
				StringList::const_iterator it2 = values.begin();
				string value = *it2;
//				for (; it2 != values.end(); it2++) {
//
//					SYSLOGLOGGER(logINFO) << "   " << *it2 << "; ";
//				}
//				SYSLOGLOGGER(logINFO) << std::endl;
				//retval->addAttribute(entry->getDN(), attr.getName(), value);
				if (0 == attr.getName().compare("sstBackupNumberOfIterations")) {
					config->setIterations(atoi(value.c_str()));
				}
				else if (0 == attr.getName().compare("sstBackupExcludeFromBackup")) {
					config->setExclude(0 == value.compare("TRUE"));
				}
				else if (0 == attr.getName().compare("sstCronActive")) {
					config->setCronActive(0 == value.compare("TRUE"));
				}
				else if (0 == attr.getName().compare("sstCronDay")) {
					config->setCronDay(value);
				}
				else if (0 == attr.getName().compare("sstCronDayOfWeek")) {
					config->setCronDayOfWeek(value);
				}
				else if (0 == attr.getName().compare("sstCronHour")) {
					config->setCronHour(value);
				}
				else if (0 == attr.getName().compare("sstCronMinute")) {
					config->setCronMinute(value);
				}
				else if (0 == attr.getName().compare("sstCronMonth")) {
					config->setCronMonth(value);
				}
			}
			delete entry;
			entry = entries->getNext();
		}
	}
}

void LdapTools::readConfigurationSettings() {
	string base("ou=settings,ou=configuration,ou=virtualization,ou=services,");
	base.append(Config::getInstance()->getLdapBaseDn());
	SYSLOGLOGGER(logDEBUG) << "readConfigurationSettings " << base;
	LDAPSearchResults* entries = lc->search(base, LDAPConnection::SEARCH_SUB);
	if (entries != 0) {
		LDAPEntry* entry = entries->getNext();
		while (entry != 0) {
//			SYSLOGLOGGER(logINFO) << "dn: " << entry->getDN();
			const LDAPAttributeList* attrs = entry->getAttributes();
			LDAPAttributeList::const_iterator it = attrs->begin();
			for (; it != attrs->end(); it++) {
				LDAPAttribute attr = *it;
//				SYSLOGLOGGER(logINFO) << attr.getName() << "(";
//				SYSLOGLOGGER(logINFO) << attr.getNumValues() << "): ";
				StringList values = attr.getValues();
				StringList::const_iterator it2 = values.begin();
				string value = *it2;
//				for (; it2 != values.end(); it2++) {
//
//					SYSLOGLOGGER(logINFO) << *it2 << "; ";
//				}
//				SYSLOGLOGGER(logINFO) << std::endl;
				//retval->addAttribute(entry->getDN(), attr.getName(), value);
				if (string::npos != entry->getDN().find("ou=sound")) {
					if (0 == attr.getName().compare("sstAllowSound")) {
						Config::getInstance()->setAllowSound(0 == value.compare("TRUE"));
					}
				}
				else if (string::npos != entry->getDN().find("ou=spice")) {
					if (0 == attr.getName().compare("sstAllowSpice")) {
						Config::getInstance()->setAllowSpice(0 == value.compare("TRUE"));
					}
					else if (0 == attr.getName().compare("sstSpicePortMin")) {
						Config::getInstance()->setSpicePortMin(atoi(value.c_str()));
					}
					else if (0 == attr.getName().compare("sstSpicePortMax")) {
						Config::getInstance()->setSpicePortMax(atoi(value.c_str()));
					}
				}
				else if (string::npos != entry->getDN().find("ou=usb")) {
					if (0 == attr.getName().compare("sstAllowUSB")) {
						Config::getInstance()->setAllowUsb(0 == value.compare("TRUE"));
					}
				}
			}
			delete entry;
			entry = entries->getNext();
		}
	}
}

const string LdapTools::getFreeIp(const NetworkRange* range) {
	string retval = "";
	set<long> ips;

	string base = "ou=dhcp,ou=networks,ou=virtualization,ou=services,";
	base.append(Config::getInstance()->getLdapBaseDn());
	string filter = "(&(objectClass=sstVirtualizationNetworkRange)(cn=";
	filter.append(range->getRange()).append("))");
	StringList attrs = StringList();
	attrs.add("cn");
	SYSLOGLOGGER(logDEBUG) << "getFreeIp: " << base << "; " << filter;
	LDAPSearchResults* entries = lc->search(base, LDAPConnection::SEARCH_SUB, filter, attrs);
	if (NULL != entries) {
		LDAPEntry* entry = entries->getNext();
		if (NULL != entry) {
			string dn = entry->getDN();
			SYSLOGLOGGER(logDEBUG) << "rangeDN: " << dn;
			size_t pos = dn.find("ou=ranges,");
			dn = dn.substr(pos + 10);
			SYSLOGLOGGER(logDEBUG) << "subnetDN: " << dn;

			attrs.clear();
			attrs.add("dhcpOption");
			LDAPSearchResults* entries2 = lc->search(dn, LDAPConnection::SEARCH_SUB, "objectClass=dhcpOptions", attrs);
			if (NULL != entries2) {
				LDAPEntry* entry2 = entries2->getNext();
//				while (entry2 != 0) {
//					std::cout << "dn: " << entry2->getDN() << endl;
//					const LDAPAttributeList* attrs = entry2->getAttributes();
//					LDAPAttributeList::const_iterator it = attrs->begin();
//					for (; it != attrs->end(); it++) {
//						LDAPAttribute attr = *it;
//						std::cout << attr.getName() << "(";
//						std::cout << attr.getNumValues() << "): ";
//						StringList values = attr.getValues();
//						StringList::const_iterator it2 = values.begin();
//						for (; it2 != values.end(); it2++) {
//
//							std::cout << *it2 << "; ";
//						}
//						std::cout << std::endl;
//					}
//					std::cout << endl;
//					delete entry2;
//					entry2 = entries->getNext();
//				}

				if (NULL != entry2) {
					const LDAPAttribute* attribute = entry2->getAttributeByName("dhcpOption");
					if (NULL != attribute) {
						StringList values = attribute->getValues();
						for (StringList::const_iterator it = values.begin(); it != values.end(); it++) {
							string value = *it;
							if (0 == value.find("routers ")) {
								SYSLOGLOGGER(logDEBUG) << value;
								ips.insert(NetworkRange::ip2long(value.substr(8)));
								break;
							}
						}
					}
					delete entry2;
				}
			}

			dn = "ou=virtual machines," + dn;
			SYSLOGLOGGER(logDEBUG) << "search IPs DN: " << dn;

			filter = "(objectClass=dhcpHost)";
			attrs.clear();
			attrs.add("dhcpStatements");
			LDAPSearchResults* entries3 = lc->search(dn, LDAPConnection::SEARCH_SUB, filter, attrs);
			if (NULL != entries3) {
				LDAPEntry* entry3 = entries3->getNext();
				while (NULL != entry3) {
					StringList values = entry3->getAttributeByName("dhcpStatements")->getValues();
					for (StringList::const_iterator it = values.begin(); it != values.end(); it++) {
						string value = *it;
						if (0 == value.find("fixed-address ")) {
							SYSLOGLOGGER(logDEBUG) << value << "; " << (NetworkRange::ip2long(value.substr(14)));
							ips.insert(NetworkRange::ip2long(value.substr(14)));
						}
					}

					delete entry3;
					entry3 = entries3->getNext();
				}
			}
			delete entry;
		}
	}
	long hostmin = NetworkRange::ip2long(range->getHostMin());
	long hostmax = NetworkRange::ip2long(range->getHostMax());
	for (long i = hostmin; i <= hostmax; i++) {
		SYSLOGLOGGER(logDEBUG) << i << ": " << (NetworkRange::long2ip(i));
		if (ips.end() == ips.find(i)) {
			retval = NetworkRange::long2ip(i);
			break;
		}
	}

	return retval;
}

const int LdapTools::getGlobalSetting(const string& setting) const {
	int retval = -1;
	string base = "";
	string settingAttrName;

	if (0 == setting.compare("usb")) {
		base = string("ou=");
		base.append(setting).append(",ou=settings,ou=configuration,ou=virtualization,ou=services,").append(
				Config::getInstance()->getLdapBaseDn());
		settingAttrName = "sstAllowUSB";
	}
	else if (0 == setting.compare("sound")) {
		base = string("ou=");
		base.append(setting).append(",ou=settings,ou=configuration,ou=virtualization,ou=services,").append(
				Config::getInstance()->getLdapBaseDn());
		settingAttrName = "sstAllowSound";
	}

	if (0 < base.length()) {
		SYSLOGLOGGER(logINFO) << "getGlobalSetting " << base;
		LDAPSearchResults* entries = NULL;
		try {
			entries = lc->search(base, LDAPConnection::SEARCH_SUB);
		}
		catch (LDAPException &e) {
			SYSLOGLOGGER(logERROR) << "getGlobalSetting -------------- caught LDAPException ---------";
			SYSLOGLOGGER(logERROR) << e;
			if (32 != e.getResultCode()) {
				// No Such Object
				throw;
			}
			else {
				entries = NULL;
			}
		}
		if (NULL != entries) {
			LDAPEntry* entry = entries->getNext();
			while (entry != 0) {
				const LDAPAttributeList* attrs = entry->getAttributes();
				LDAPAttributeList::const_iterator it = attrs->begin();
				for (; -1 == retval && it != attrs->end(); it++) {
					LDAPAttribute attr = *it;
	//				SYSLOGLOGGER(logINFO) << attr.getName() << "(" << attr.getNumValues() << "): ";
					StringList values = attr.getValues();
					StringList::const_iterator it2 = values.begin();
					string value = *it2;
	//				for (; it2 != values.end(); it2++) {
	//					SYSLOGLOGGER(logINFO) << *it2 << "; ";
	//				}
					if (0 == attr.getName().compare(settingAttrName)) {
						retval = 0 == value.compare("TRUE") ? 1 : 0;
					}
				}
				delete entry;
				entry = entries->getNext();
			}
		}
	}
	return retval;
}
