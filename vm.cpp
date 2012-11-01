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
 * vm.cpp
 *
 *  Created on: 11.05.2012
 *      Author: cwi
 */

#include <cstdio>
#include <set>
#include <algorithm>
#include <boost/algorithm/string.hpp>

#include "LDAPModList.h"

#include "include/vm.hpp"
#include "include/vmPool.hpp"
#include "include/node.hpp"
#include "include/virtTools.hpp"
#include "include/ldapTools.hpp"
#include "include/logger.hpp"

using namespace std;

void Vm::remove() {
	SYSLOGLOGGER(logDEBUG) << "remove Vm: " << getName();
	VmPool* vmPool = const_cast<VmPool*>(getVmPool());
	vmPool->removeVm(this);
//	Config::getInstance()->removeVm(this);
	const VmDeviceDisk* disk = getDiskByDeviceName("vda");
	if (NULL != disk) {
		if (0 != ::remove(disk->getSourceFile().c_str())) {
			SYSLOGLOGGER(logDEBUG) << "Unable to remove file " << disk->getSourceFile();
		}
		else {
			SYSLOGLOGGER(logDEBUG) << "'vd' disk removed: " << disk->getSourceFile();
		}
	}
	else {
		SYSLOGLOGGER(logDEBUG) << "Unable to find 'vda' disk";
	}

	lt->removeEntry(getDn(), true);
	const NetworkRange* range = this->getVmPool()->getRange();
	string dn_ = "cn=";
	string dnPart;
	getDnParent(range->getDn(), dnPart, 2);
	dn_.append(getName()).append(",ou=virtual machines,").append(dnPart);
	lt->removeEntry(dn_);
	SYSLOGLOGGER(logDEBUG) << "remove Vm finished!";
}

void Vm::migrate(const Node* targetNode, VirtTools* vt) {
	try {
		const Vm* vm = this;
		vt->migrateVm(vm, targetNode);

		LDAPModList* modlist = new LDAPModList();
		const string targetNodeName = targetNode->getName();
		LDAPAttribute attr = LDAPAttribute("sstNode", targetNodeName);
		LDAPModification modification = LDAPModification(attr, LDAPModification::OP_REPLACE);
		modlist->addModification(modification);
		SYSLOGLOGGER(logDEBUG) << (getDn()) << ": change sstNode to " << targetNodeName;
		lt->modifyEntry(getDn(), modlist);
		delete modlist;

		/* the following lines will be done in caller method migrateFirstOne of class VmPoolNodeWrapper */
/*
		const_cast<Node*>(targetNode)->addVm(this);

		Node* node = const_cast<Node*>(getNode());
		set<Vm*>* nodeVms = node->getVms();
		nodeVms->erase(this);
*/
	}
	catch (VirtException &e) {
		SYSLOGLOGGER(logDEBUG) << "-------------- caught VirtException ---------";
		SYSLOGLOGGER(logDEBUG) << e;
	}
	catch (LDAPException &e) {
		SYSLOGLOGGER(logDEBUG) << "-------------- caught LDAPException ---------";
		SYSLOGLOGGER(logDEBUG) << e;
	}
}

const VmDeviceDisk* Vm::getDiskByDeviceName(const string& deviceName) const {
	const VmDeviceDisk* retval = NULL;

	map<string, VmDeviceDisk*>::const_iterator it = disks.begin();
	for (; it != disks.end(); it++) {
		const VmDeviceDisk* disk = it->second;
		if (0 == deviceName.compare(disk->getName())) {
			retval = disk;
			break;
		}
	}
	return retval;
}

bool Vm::addAttribute(const string& actDn, const string& attr, const string& val) {
	if (0 == actDn.compare(this->dn)) {
		//SYSLOGLOGGER(logDEBUG) << "addAttribute: Vm " << attr;
		if (0 == attr.compare("sstBelongsToCustomerUID")) {
			customerUID = string(val);
		}
		else if (0 == attr.compare("sstBelongsToResellerUID")) {
			resellerUID = string(val);
		}
		else if (0 == attr.compare("sstVirtualMachine")) {
			name = string(val);
		}
		else if (0 == attr.compare("sstDisplayName")) {
			displayName = string(val);
		}
		else if (0 == attr.compare("sstNode")) {
			nodeName = string(val);
		}
		else if (0 == attr.compare("sstVirtualMachinePool")) {
			vmPoolName = string(val);
		}
		else if (0 == attr.compare("sstVirtualMachineType")) {
			vmType = string(val);
		}
		else if (0 == attr.compare("sstVirtualMachineSubType")) {
			vmSubType = string(val);
		}
		else if (0 == attr.compare("sstClockOffset")) {
			clockOffset = string(val);
		}
		else if (0 == attr.compare("sstMemory")) {
			memory = string(val);
		}
		else if (0 == attr.compare("sstOnCrash")) {
			onCrash = string(val);
		}
		else if (0 == attr.compare("sstOnPowerOff")) {
			onPowerOff = string(val);
		}
		else if (0 == attr.compare("sstOnReboot")) {
			onReboot = string(val);
		}
		else if (0 == attr.compare("sstOSArchitecture")) {
			osArchitecture = string(val);
		}
		else if (0 == attr.compare("sstOSBootDevice")) {
			osBootDevice = string(val);
		}
		else if (0 == attr.compare("sstOSMachine")) {
			osMachine = string(val);
		}
		else if (0 == attr.compare("sstOSType")) {
			osType = string(val);
		}
		else if (0 == attr.compare("sstType")) {
			type = string(val);
		}
		else if (0 == attr.compare("sstVCPU")) {
			vCpu = string(val);
		}
		else if (0 == attr.compare("sstSpicePort")) {
			spicePort = string(val);
		}
		else if (0 == attr.compare("sstSpicePassword")) {
			spicePassword = string(val);
		}
	}
	else if (string::npos != actDn.find(",ou=people")) {
		//SYSLOGLOGGER(logDEBUG) << "addAttribute: Vm People " << attr;
		if (0 == attr.compare("ou")) {
			user = string(val);
		}
	}
	else if (string::npos != actDn.find(",ou=devices")) {
		if (string::npos != actDn.find("sstDisk=")) {
			//SYSLOGLOGGER(logDEBUG) << "addAttribute: Vm Devices Disk " << attr;
			string diskName;
			getDnPart(actDn, "sstDisk", diskName);
			map<string, VmDeviceDisk*>::iterator it = disks.find(diskName);
			VmDeviceDisk* disk = NULL;
			if (disks.end() != it) {
				disk = it->second;
			}
			else {
				disk = new VmDeviceDisk();
				disks[diskName] = disk;
			}
			if (0 == attr.compare("sstDisk")) {
				disk->setName(val);
			}
			else if (0 == attr.compare("sstDevice")) {
				disk->setDevice(val);
			}
			else if (0 == attr.compare("sstSourceFile")) {
				disk->setSourceFile(val);
			}
			else if (0 == attr.compare("sstTargetBus")) {
				disk->setTargetBus(val);
			}
			else if (0 == attr.compare("sstType")) {
				disk->setType(val);
			}
			else if (0 == attr.compare("sstReadonly")) {
				string upper = boost::to_upper_copy(val);
				disk->setReadonly(0 == upper.compare("TRUE"));
			}
			else if (0 == attr.compare("sstVolumeName")) {
				disk->setVolumeName(val);
			}
			else if (0 == attr.compare("sstVolumeCapacity")) {
				disk->setVolumeCapacity(atol(val.c_str()));
			}
			else if (0 == attr.compare("sstDriverCache")) {
				disk->setDriverCache(val);
			}
			else if (0 == attr.compare("sstDriverName")) {
				disk->setDriverName(val);
			}
			else if (0 == attr.compare("sstDriverType")) {
				disk->setDriverType(val);
			}
		}
		else if (string::npos != actDn.find("sstInterface=")) {
			//SYSLOGLOGGER(logDEBUG) << "addAttribute: Vm Devices Interface " << attr;
			string interfaceName;
			getDnPart(actDn, "sstInterface", interfaceName);
			map<string, VmDeviceInterface*>::iterator it = interfaces.find(interfaceName);
			VmDeviceInterface* interface = NULL;
			if (interfaces.end() != it) {
				interface = it->second;
			}
			else {
				interface = new VmDeviceInterface();
				interfaces[interfaceName] = interface;
			}
			if (0 == attr.compare("sstInterface")) {
				interface->setName(val);
			}
			else if (0 == attr.compare("sstType")) {
				interface->setType(val);
			}
			else if (0 == attr.compare("sstMacAddress")) {
				interface->setMacAddress(val);
			}
			else if (0 == attr.compare("sstModelType")) {
				interface->setModelType(val);
			}
			else if (0 == attr.compare("sstSourceBridge")) {
				interface->setSourceBridge(val);
			}
		}
	}
	else if (string::npos != actDn.find("ou=devices")) {
		if (0 == attr.compare("sstEmulator")) {
			emulator = string(val);
		}
		else if (0 == attr.compare("sstMemBalloon")) {
			memBalloon = string(val);
		}
	}
	return true;
}

ostream& operator <<(ostream& s, const Vm& vm) {
	s << vm.displayName << ", " << vm.name << " (" << vm.vmType << ", " << vm.vmSubType << ") ";
	switch (vm.status) {
		case VmCheckAgain:
			s << "CHECK AGAIN";
			break;
		case VmRunning:
			s << "RUNNING";
			break;
		default:
			s << "UNKNOWN";
			break;
	}
	if (0 < vm.user.length()) {
		s << " for user " << vm.user;
	}
	else if (vm.status == VmRunning) {
		s << " prestarted";
	}
	s << std::endl << "+-> Node: " << vm.nodeName;
	s << std::endl << "+-> Disks:" << std::endl;
	for (map<string, VmDeviceDisk*>::const_iterator it = vm.disks.begin(); it != vm.disks.end(); it++) {
		VmDeviceDisk* disk = (*it).second;
		s << "    +-> " << *disk;
	}
	return s;
}

ostream& operator <<(ostream& s, const VmDeviceDisk& vmDeviceDisk) {
	s << vmDeviceDisk.name << " (" << vmDeviceDisk.type << ", " << vmDeviceDisk.device << "); " << std::endl;
	s << "        +-> Source: " << vmDeviceDisk.sourceFile << std::endl;
	if (0 < vmDeviceDisk.volumeName.length()) {
		s << "        +-> Volume: " << vmDeviceDisk.volumeName << std::endl;
	}
	return s;
}

ostream& operator <<(ostream& s, const VmDeviceInterface& vmDeviceInterface) {
	s << vmDeviceInterface.name << " (" << vmDeviceInterface.type << ", " << vmDeviceInterface.modelType << "); " << std::endl;
	s << "        +-> MacAdress: " << vmDeviceInterface.macAddress << std::endl;
	s << "        +-> SourceBridge: " << vmDeviceInterface.sourceBridge << std::endl;

	return s;
}

