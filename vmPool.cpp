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
 * vmPool.cpp
 *
 *  Created on: 11.05.2012
 *      Author: cwi
 */

#include <string>

#include "include/config.hpp"
#include "include/ldapTools.hpp"
#include "include/virtTools.hpp"
#include "include/vmPool.hpp"
#include "include/vm.hpp"
#include "include/node.hpp"
#include "include/networkRange.hpp"
#include "include/logger.hpp"

using namespace std;

bool VmPool::addAttribute(const string& actDn, const string& attr, const string& val) {
	if (0 == actDn.compare(this->dn)) {
		//SYSLOGLOGGER(logDEBUG) << "addAttribute: VmPool " << attr;
		if (0 == attr.compare("sstBelongsToCustomerUID")) {
			customerUID = string(val);
		}
		else if (0 == attr.compare("sstBelongsToResellerUID")) {
			resellerUID = string(val);
		}
		else if (0 == attr.compare("sstVirtualMachinePool")) {
			name = string(val);
		}
		else if (0 == attr.compare("sstDisplayName")) {
			displayName = string(val);
		}
		else if (0 == attr.compare("sstVirtualMachinePoolType")) {
			type = string(val);
		}
		else if (0 == attr.compare("sstActiveGoldenImage")) {
			goldenImage = lt->readVm(val, false);
		}
		else if (0 == attr.compare("sstBrokerMaximalNumberOfVirtualMachines")) {
			reinterpret_cast<EvenlyPolicy*>(policy)->setMaximalNumberOfVirtualMachines(atoi(val.c_str()));
		}
		else if (0 == attr.compare("sstBrokerMinimalNumberOfVirtualMachines")) {
			reinterpret_cast<EvenlyPolicy*>(policy)->setMinimalNumberOfVirtualMachines(atoi(val.c_str()));
		}
		else if (0 == attr.compare("sstBrokerPreStartNumberOfVirtualMachines")) {
			reinterpret_cast<EvenlyPolicy*>(policy)->setPreStartNumberOfVirtualMachines(atoi(val.c_str()));
		}
	}
	else if (string::npos != actDn.find(",ou=nodes")) {
		//SYSLOGLOGGER(logDEBUG)  << "addAttribute: VmPool Node " << attr;
		string nodeName;
		getDnPart(actDn, "ou", nodeName);

		map<string, Node*>* allNodes = Config::getInstance()->getNodes();
		map<string, Node*>::iterator it = allNodes->find(nodeName);
		Node* node = NULL;
		if (allNodes->end() != it) {
//			SYSLOGLOGGER(logDEBUG)  << "Node found";
			node = (*it).second;
		}
		else {
			SYSLOGLOGGER(logDEBUG)  << "new Node " << nodeName << endl;
			node = lt->readNode(nodeName);
			Config::getInstance()->addNode(node);
		}
		if (0 == attr.compare("ou")) {
			addNode(node);
		}
	}
	else if (string::npos != actDn.find(",ou=ranges")) {
		//SYSLOGLOGGER(logDEBUG)  << "addAttribute: VmPool Range " << attr;
		if (0 == attr.compare("ou")) {
			string dn_ = lt->getNetworkRangeDn(val);

			range = new NetworkRange(actDn, dn_, val);
		}
	}
	else if (string::npos != actDn.find(",ou=storage pools")) {
		//SYSLOGLOGGER(logDEBUG)  << "addAttribute: VmPool StoragePool " << attr;
		if (0 == attr.compare("ou")) {
			storagePoolName = string(val);
			storagePoolDir = lt->readStoragePoolUri(storagePoolName).substr(7);
		}
	}
	else if (string::npos != actDn.find("ou=backup")) {
		if (0 == attr.compare("sstBackupNumberOfIterations")) {
			backupConfiguration.setIterations(atoi(val.c_str()));
		}
		else if (0 == attr.compare("sstBackupExcludeFromBackup")) {
			backupConfiguration.setExclude(0 == val.compare("TRUE"));
		}
		else if (0 == attr.compare("sstCronActive")) {
			backupConfiguration.setCronActive(0 == val.compare("TRUE"));
		}
		else if (0 == attr.compare("sstCronDay")) {
			backupConfiguration.setCronDay(val);
		}
		else if (0 == attr.compare("sstCronDayOfWeek")) {
			backupConfiguration.setCronDayOfWeek(val);
		}
		else if (0 == attr.compare("sstCronHour")) {
			backupConfiguration.setCronHour(val);
		}
		else if (0 == attr.compare("sstCronMinute")) {
			backupConfiguration.setCronMinute(val);
		}
		else if (0 == attr.compare("sstCronMonth")) {
			backupConfiguration.setCronMonth(val);
		}
	}
	return true;
}

bool VmPool::hasActiveGoldenImage() {
	return NULL != goldenImage;
}

void VmPool::addVm(Vm* vm) {
	SYSLOGLOGGER(logDEBUG)  << "VmPool::addVm: " << vm->getName() << "; " << vm->getNodeName();
	vms[vm->getName()] = vm;

	nodeWrappers[vm->getNodeName()]->addVm(vm);
}

void VmPool::removeVm(Vm* vm)
{
	SYSLOGLOGGER(logDEBUG)  << "VmPool::removeVm: " << vm->getName() << "; " << vm->getNodeName();
	vms.erase(vm->getName());
	nodeWrappers[vm->getNodeName()]->removeVm(vm);
}

void VmPool::addNode(Node* node) {
	SYSLOGLOGGER(logDEBUG)  << "VmPool::addNode: " << node->getName();
	nodeWrappers[node->getName()] = new VmPoolNodeWrapper(node);
}

ostream& operator <<(ostream& s, const VmPool& vmPool) {
	s << "+-> " << vmPool.name << " (" << vmPool.displayName << ")" << endl;
	s << "   +-> StoragePool: " << vmPool.storagePoolName << ", " << vmPool.storagePoolDir << endl;
	s << "   +-> Range: " << *vmPool.range << endl;
	s << "   +-> Golden: ";
	if (NULL != vmPool.goldenImage) {
		s << vmPool.goldenImage->getName() << " (" << vmPool.goldenImage->getDisplayName() << ")";
	}
	s << endl;
	EvenlyPolicy* epolicy = reinterpret_cast<EvenlyPolicy*>(vmPool.policy);
	s << "   +-> Policy: " << (*epolicy) << endl;
	s << "   +-> Nodes:" << endl;
	for (map<string, VmPoolNodeWrapper*>::const_iterator it = vmPool.nodeWrappers.begin(); it != vmPool.nodeWrappers.end(); it++) {
		s << "     " << (*it).first << endl;
	}
	s << "   +-> Vms:" << endl;
	for (map<string, Vm*>::const_iterator itVms = vmPool.vms.begin(); itVms != vmPool.vms.end(); itVms++) {
		s << "     " << (*itVms).first << endl;
	}

	return s;
}

const std::string& VmPoolNodeWrapper::getName() const
{
	return node->getName();
}

int VmPoolNodeWrapper::migrateFirstVm(VmPoolNodeWrapper* targetWrapper, VirtTools* vt) {
	if (vms.begin() != vms.end()) {
		Vm* vm = *(vms.begin());
		removeVm(vm);

// CWI
		vm->migrate(targetWrapper->getNode(), vt);

		targetWrapper->addVm(vm);
	}
	return 0;
}

ostream& operator <<(ostream& s, const VmPoolNodeWrapper& nodeWrapper) {
	return s;
}
