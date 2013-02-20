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
 * virtTools.cpp
 *
 *  Created on: 31.05.2012
 *      Author: cwi
 */

#include <iostream>
#include <sstream>

#include "include/config.hpp"
#include "include/virtTools.hpp"
#include "include/vm.hpp"
#include "include/vmPool.hpp"
#include "include/node.hpp"
#include "include/logger.hpp"

extern "C" {
#include <malloc.h>
#include "libvirt/libvirt.h"
}

using namespace std;

virConnectPtr VirtTools::getConnection(const string nodeUri) {
	virConnectPtr retval = NULL;
	std::map<std::string, virConnectPtr>::const_iterator it = connections.find(nodeUri);
	if (connections.end() != it) {
		retval = it->second;
	}
	else {
		retval = virConnectOpen(nodeUri.c_str());
		if (retval != NULL) {
			connections[nodeUri] = retval;
		}
	}

	return retval;
}

bool VirtTools::checkVmsPerNode() {
	bool retval = true;
	map<string, Node*>* nodes = Config::getInstance()->getNodes();
	map<string, Node*>::const_iterator it = nodes->begin();
	for (; it != nodes->end(); it++) {
		retval &= checkVmsByNode(it->second);
	}

	if (retval) {
		map<string, Vm*>* vms = Config::getInstance()->getVms();
		Vm* vm = NULL;
		map<string, Vm*>::iterator it2 = vms->begin();
		while(it2!=vms->end()) {
			vm = it2->second;
			if (VmStatusUnknown == vm->getStatus()) {
				vm->remove();
				vms->erase(it2++);
				delete vm;
			}
			else {
				it2++;
			}
		}
	}
	return retval;
}

bool VirtTools::checkVmsByNode(Node* node) {
	bool retval = true;

	string nodeUri = node->getVirtUri();
	try {
		virConnectPtr conn;
		conn = getConnection(nodeUri);
		if (conn == NULL) {
			string message = "Failed to open connection to ";
			message.append(nodeUri);
			throw VirtException(message);
		}
		int numDomains = virConnectNumOfDomains(conn);
		if (-1 == numDomains) {
			string message = "Failed to get number of active domains from ";
			message.append(nodeUri);
			throw VirtException(message);
		}
		int* activeDomains = reinterpret_cast<int*>(malloc(sizeof(char *) * numDomains));
		numDomains = virConnectListDomains(conn, activeDomains, numDomains);
		if (-1 == numDomains) {
			string message = "Failed to get ids of active domains from ";
			message.append(nodeUri);
			throw VirtException(message);
		}
		virDomainPtr domain;
		SYSLOGLOGGER(logINFO) << "vt: Domains (" << numDomains << ") running on " << node->getName() << ": ";
		for (int i = 0; i < numDomains; i++) {
			domain = virDomainLookupByID(conn, activeDomains[i]);
			if (NULL != domain) {
				const char* dName = virDomainGetName(domain);
				if (NULL != dName) {
					string domainName = string(dName);
					SYSLOGLOGGER(logINFO) << "vt:    " << activeDomains[i] << "; " << domainName;

					map<string, Vm*>::iterator it = Config::getInstance()->getVms()->find(domainName);
					Vm* vm = NULL;
					if (Config::getInstance()->getVms()->end() != it) {
						vm = it->second;
						vm->setStatus(VmRunning);
						const_cast<VmPoolNodeWrapper*>(vm->getVmPool()->getNodeWrapper(node->getName()))->addVm(vm);
					}
				}
				if (-1 == virDomainFree(domain)) {
					SYSLOGLOGGER(logWARNING) << "vt: Unable to free domain object";
				}
			}
		}
		free(reinterpret_cast<void*>(activeDomains));
	}
	catch(VirtException &e) {
		SYSLOGLOGGER(logINFO) << "-------------- caught VirtException ---------";
		SYSLOGLOGGER(logINFO) << e;
		SYSLOGLOGGER(logINFO) << "------------ try it in next loop ------------";
		// set all domains that have status=VmStatusUnknown to status=VmCheckAgain
		map<string, Vm*>* vms = Config::getInstance()->getVms();
		Vm* vm = NULL;
		map<string, Vm*>::iterator it = vms->begin();
		for(; it!=vms->end(); it++) {
			vm = it->second;
			if (0 == vm->getNodeName().compare(node->getName()) && VmStatusUnknown == vm->getStatus()) {
				vm->setStatus(VmCheckAgain);
				retval = false;
			}
		}
	}

	return retval;
}

const string VirtTools::getVmXml(const Vm* vm) const {
	ostringstream buffer(ostringstream::out);
	buffer << "<domain type=\"" << vm->getType() << "\">" << endl;
	buffer << "\t<name>" << vm->getName() << "</name>" << endl;
	buffer << "\t<uuid>" << vm->getName() << "</uuid>" << endl;
	buffer << "\t<memory>" << vm->getMemoryKb() << "</memory>" << endl;
	buffer << "\t<vcpu>" << vm->getVCpu() << "</vcpu>" << endl;
	buffer << "\t<os>" << endl;
	buffer << "\t\t<type arch=\"" << vm->getOsArchitecture() << "\" machine=\"" << vm->getOsMachine() << "\">"
			<< vm->getOsType() << "</type>" << endl;
	buffer << "\t\t<boot dev=\"" << vm->getOsBootDevice() << "\"/>" << endl;
	buffer << "\t</os>" << endl;
	buffer << "\t<features>" << endl;
	StringList::const_iterator itList = vm->getFeatures().begin();
	for (; itList != vm->getFeatures().end(); itList++) {
		buffer << "\t\t<" << *itList << "/>" << endl;
	}
	buffer << "\t</features>" << endl;
	buffer << "\t<clock offset=\"" << vm->getClockOffset() << "\"/>" << endl;
	buffer << "\t<on_poweroff>" << vm->getOnPowerOff() << "</on_poweroff>" << endl;
	buffer << "\t<on_reboot>" << vm->getOnReboot() << "</on_reboot>" << endl;
	buffer << "\t<on_crash>" << vm->getOnCrash() << "</on_crash>" << endl;
	buffer << "\t<devices>" << endl;
	buffer << "\t\t<emulator>" << vm->getEmulator() << "</emulator>" << endl;
	buffer << "\t\t<graphics type=\"spice\" port=\"" << vm->getSpicePort()
			<< "\" tlsPort=\"0\" autoport=\"no\" listen=\"0.0.0.0\" passwd=\"" << vm->getSpicePassword() << "\">"
			<< endl;
	buffer << "\t\t\t<listen type=\"address\" address=\"" << vm->getNode()->getVLanIP("pub")  << "\" />" << endl;
	buffer << "\t\t</graphics>" << endl;
//	buffer << "\t\t</graphics>" << endl;
	buffer << "\t\t<channel type=\"spicevmc\">" << endl;
	buffer << "	\t\t\t<target type=\"virtio\" name=\"com.redhat.spice.0\"/>" << endl;
	buffer << "\t\t</channel>" << endl;
	buffer << "	\t\t<video>" << endl;
	buffer << "	\t\t<model type=\"qxl\" vram=\"65536\" heads=\"1\"/>" << endl;
	buffer << "	\t\t</video>" << endl;
	buffer << "	\t\t<input type=\"tablet\" bus=\"usb\"/>" << endl;
	buffer << "\t\t<sound model=\"ac97\"/>" << endl;
	map<string, VmDeviceDisk*>::const_iterator it = vm->getDisks()->begin();
	for (; it != vm->getDisks()->end(); it++) {
		VmDeviceDisk* disk = it->second;

		buffer << "\t\t<disk type=\"" << disk->getType() << "\" device=\"" << disk->getDevice() << "\">" << endl;
		if (0 < disk->getDriverName().length() && 0 < disk->getDriverType().length()) {
			buffer << "\t\t\t<driver name=\"" << disk->getDriverName() << "\" type=\"" << disk->getDriverType() << "\"";
			if (0 < disk->getDriverCache().length()) {
				buffer << " cache=\"" << disk->getDriverCache() << "\"/>" << endl;
			}
		}
		buffer << "\t\t\t<source file=\"" << disk->getSourceFile() << "\"/>" << endl;
		buffer << "\t\t\t<target dev=\"" << disk->getName() << "\" bus=\"" << disk->getTargetBus() << "\"/>" << endl;
		if (disk->isReadonly()) {
			buffer << "\t\t\t<readOnly/>" << endl;
		}
		buffer << "\t\t</disk>" << endl;
	}
	map<string, VmDeviceInterface*>::const_iterator it2 = vm->getInterfaces()->begin();
	for (; it2 != vm->getInterfaces()->end(); it2++) {
		VmDeviceInterface* interface = it2->second;

		buffer << "\t\t<interface type=\"" << interface->getType() << "\">" << endl;
		buffer << "\t\t\t<source bridge=\"" << interface->getSourceBridge() << "\"/>" << endl;
		buffer << "\t\t\t<mac address=\"" << interface->getMacAddress() << "\"/>" << endl;
		buffer << "\t\t\t<model type=\"" << interface->getModelType() << "\"/>" << endl;
		buffer << "\t\t</interface>" << endl;
	}

	buffer << "\t</devices>" << endl;
	buffer << "</domain>" << endl;

	return buffer.str();
}

const string VirtTools::getBackingStoreVolumeXML(const Vm* vm, const string& volumeName) const {
	ostringstream buffer(ostringstream::out);
	buffer << "<volume>" << endl;
	buffer << "\t<name>" << volumeName << ".qcow2</name>" << endl;
	buffer << "\t<allocation>0</allocation>" << endl;

	const VmDeviceDisk* disk = vm->getDiskByDeviceName("vda");
	if (NULL == disk) {
		throw VirtException("DeviceDisk 'vda' not found in VM");
	}
	buffer << "\t<capacity>" << (disk->getVolumeCapacity()) << "</capacity>" << endl;
	buffer << "\t<backingStore>" << endl;
	buffer << "\t\t<path>" << (disk->getVolumeName()) << ".qcow2</path>" << endl;
	buffer << "\t\t<format type=\"qcow2\"/>" << endl;
	buffer << "\t</backingStore>" << endl;
	buffer << "\t<target>" << endl;
	buffer << "\t\t<format type=\"qcow2\"/>" << endl;
	buffer << "\t\t<permissions>" << endl;
	buffer << "\t\t\t<owner>0</owner>" << endl;
	buffer << "\t\t\t<group>3000</group>" << endl;
	buffer << "\t\t\t<mode>0660</mode>" << endl;
	buffer << "\t\t</permissions>" << endl;
	buffer << "\t</target>" << endl;
	buffer << "</volume>" << endl;

	return buffer.str();
}

void VirtTools::createBackingStoreVolumeFile(const Vm* vm, const string& storagePoolName,
		const string& volumeName) {
	string nodeUri = vm->getNode()->getVirtUri();
	virConnectPtr conn;
	conn = getConnection(nodeUri);
	if (conn == NULL) {
		string message = "Failed to open connection to ";
		message.append(nodeUri);
		throw VirtException(message);
	}

	virStoragePoolPtr pool;
	pool = virStoragePoolLookupByUUIDString(conn, storagePoolName.c_str());
	if (pool == NULL) {
		//virConnectClose(conn);
		throw VirtException("Failed to open storagePool ");
	}

	virStorageVolPtr volume;
	volume = virStorageVolCreateXML(pool, getBackingStoreVolumeXML(vm, volumeName).c_str(), 0);
	if (volume == NULL) {
		if (-1 == virStoragePoolFree(pool)) {
			SYSLOGLOGGER(logWARNING) << "vt: Unable to free storgepool object";
		}
		//virConnectClose(conn);
		throw VirtException("Failed to create Volume ");
	}
	if (-1 == virStoragePoolFree(pool)) {
		SYSLOGLOGGER(logWARNING) << "vt: Unable to free storgepool object";
	}
	if (-1 == virStorageVolFree(volume)) {
		SYSLOGLOGGER(logWARNING) << "vt: Unable to free storgevolume object";
	}
	//virConnectClose(conn);
}

void VirtTools::startVm(const Vm* vm) {
	const Node* vmNode = vm->getNode();
	if (NULL == vmNode) {
		string message = "Failed to get Node ";
		message.append(vmNode->getVirtUri());
		throw VirtException(message);
	}
	virConnectPtr conn;
	conn = getConnection(vmNode->getVirtUri());
	if (conn == NULL) {
		string message = "Failed to open connection to ";
		message.append(vmNode->getVirtUri());
		throw VirtException(message);
	}

	virDomainPtr domain = virDomainCreateXML(conn, getVmXml(vm).c_str(), 0);
	if (domain == NULL) {
		//virConnectClose(conn);
		throw VirtException("Failed to create domain");
	}

	if (-1 == virDomainFree(domain)) {
		SYSLOGLOGGER(logWARNING) << "vt: Unable to free domain object";
	}
	//virConnectClose(conn);
}

void VirtTools::stopVmForRestore(const Vm* vm) {
	const Node* vmNode = vm->getNode();
	if (NULL == vmNode) {
		string message = "Failed to get Node ";
		message.append(vmNode->getVirtUri());
		throw VirtException(message);
	}
	virConnectPtr conn;
	conn = getConnection(vmNode->getVirtUri());
	if (conn == NULL) {
		string message = "Failed to open connection to ";
		message.append(vmNode->getVirtUri());
		throw VirtException(message);
	}
	virDomainPtr domain = virDomainLookupByName(conn, vm->getName().c_str());
	if (domain == NULL) {
		string message = "Failed to find domain ";
		message.append(vm->getName()).append(" on ").append(vmNode->getVirtUri());
		//virConnectClose(conn);
		throw VirtException(message);
	}

	SYSLOGLOGGER(logWARNING) << "vt.stopVmForRestore: " << vm->getName();
	int retval = virDomainIsActive(domain);
	if (0 > retval) {
		string message = "Failed to get domain state from ";
		message.append(vm->getName());
		throw VirtException(message);
	}
	else if (1 == retval) {
		retval = virDomainDestroy(domain);
		if (0 != retval) {
			string message = "Failed to destroy domain ";
			message.append(vm->getName());
			throw VirtException(message);
		}
	}

	retval = virDomainUndefine(domain);
	if (0 != retval) {
		string message = "Failed to undefine domain ";
		message.append(vm->getName());
		throw VirtException(message);
	}

	if (-1 == virDomainFree(domain)) {
		SYSLOGLOGGER(logWARNING) << "vt: Unable to free domain object";
	}
	//virConnectClose(conn);
}

void VirtTools::migrateVm(const Vm* vm, const Node* node, const string spicePort) {
	const Node* vmNode = vm->getNode();
	if (NULL == vmNode) {
		string message = "Failed to get Node ";
		message.append(vmNode->getVirtUri());
		throw VirtException(message);
	}

	virConnectPtr conn;
	conn = getConnection(vmNode->getVirtUri());
	if (conn == NULL) {
		string message = "Failed to open connection to ";
		message.append(vmNode->getVirtUri());
		throw VirtException(message);
	}
	virDomainPtr domain = virDomainLookupByName(conn, vm->getName().c_str());
	if (domain == NULL) {
		string message = "Failed to find domain ";
		message.append(vm->getName()).append(" on ").append(vmNode->getVirtUri());
		//virConnectClose(conn);
		throw VirtException(message);
	}

	char* xmlstr = virDomainGetXMLDesc(domain, 0);
	if (xmlstr == NULL) {
		string message = "Failed to get XML from domain ";
		message.append(vm->getName()).append(" on ").append(vmNode->getVirtUri());
		//virConnectClose(conn);
		throw VirtException(message);
	}
	string xml = (xmlstr);
	string listen = node->getVLanIP("pub");
	size_t f1 = xml.find("<graphics");
	if (string::npos != f1) {
		size_t f2 = xml.find("</graphics>", f1 + 1);
		if (string::npos != f2)  {
			size_t start, end;
			size_t f3 = xml.find("port='", f1 + 1);
			if (string::npos != f3 && f3 < f2) {
				start = f3 + 6;
				end = xml.find("'", start);
				if (string::npos != end) {
					xml.replace(start, end - start, spicePort);
				}
			}
			f3 = xml.find("listen='", f1 + 1);
			if (string::npos != f3 && f3 < f2) {
				start = f3 + 8;
				end = xml.find("'", start);
				if (string::npos != end) {
					xml.replace(start, end - start, listen);
				}
			}
			f3 = xml.find("address='", f1 + 1);
			if (string::npos != f3 && f3 < f2) {
				start = f3 + 9;
				end = xml.find("'", start);
				if (string::npos != end) {
					xml.replace(start, end - start, listen);
				}
			}
		}
	}

	delete [] xmlstr;

	unsigned long flags = VIR_MIGRATE_LIVE | VIR_MIGRATE_UNDEFINE_SOURCE | VIR_MIGRATE_PEER2PEER | VIR_MIGRATE_TUNNELLED | VIR_MIGRATE_PERSIST_DEST | 512; // | VIR_MIGRATE_UNSAFE;
	if (-1 == virDomainMigrateToURI2(domain, node->getVirtUri().c_str(), NULL, xml.c_str(), flags, vm->getName().c_str(), 0)) {
		if (-1 == virDomainFree(domain)) {
			SYSLOGLOGGER(logWARNING) << "vt: Unable to free domain object for " << vm->getName();
		}
		string message = "Failed to migrate domain ";
		message.append(vm->getName()).append(" from ").append(vmNode->getVirtUri()).append(" to ").append(node->getVirtUri());
		//virConnectClose(conn);
		throw VirtException(message);
	}

	/*
	unsigned long flags = VIR_MIGRATE_LIVE | VIR_MIGRATE_UNDEFINE_SOURCE | VIR_MIGRATE_PEER2PEER | VIR_MIGRATE_TUNNELLED;
	if (-1 == virDomainMigrateToURI(domain, node->getVirtUri().c_str(), flags, vm->getName().c_str(), 0)) {
		if (-1 == virDomainFree(domain)) {
			SYSLOGLOGGER(logWARNING) << "vt: Unable to free domain object for " << vm->getName();
		}
		string message = "Failed to migrate domain ";
		message.append(vm->getName()).append(" from ").append(vmNode->getVirtUri()).append(" to ").append(node->getVirtUri());
		//virConnectClose(conn);
		throw VirtException(message);
	}
*/
	if (-1 == virDomainFree(domain)) {
		SYSLOGLOGGER(logWARNING) << "vt: Unable to free domain object for " << vm->getName();
	}
	//virConnectClose(conn);
}

ostream& operator <<(ostream& s, const VirtException e) {
	s << "Error " << e.what();
	return s;
}
