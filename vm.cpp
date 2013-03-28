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
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/conversion.hpp>

#include "LDAPModList.h"

#include "include/vm.hpp"
#include "include/vmPool.hpp"
#include "include/node.hpp"
#include "include/virtTools.hpp"
#include "include/ldapTools.hpp"
#include "include/logger.hpp"

using namespace std;
using namespace boost::gregorian;
using namespace boost::posix_time;

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

		LDAPModList* modlist = new LDAPModList();
		const string targetNodeName = targetNode->getName();
		const string targetSpicePort = lt->nextSpicePort(targetNode);
		LDAPAttribute attr = LDAPAttribute("sstMigrationNode", targetNodeName);
		LDAPModification modification = LDAPModification(attr, LDAPModification::OP_ADD);
		modlist->addModification(modification);
		attr = LDAPAttribute("sstMigrationSpicePort", targetSpicePort);
		modification = LDAPModification(attr, LDAPModification::OP_ADD);
		modlist->addModification(modification);
		SYSLOGLOGGER(logDEBUG) << (getName()) << ": set sstMigrationNode to " << targetNodeName;
		SYSLOGLOGGER(logDEBUG) << (getName()) << ": set sstMigrationSpicePort to " << targetSpicePort;
		try {
			lt->modifyEntry(getDn(), modlist);
			delete modlist;
		}
		catch(LDAPException &e) {
			delete modlist;
			modlist = new LDAPModList();
			attr = LDAPAttribute("sstMigrationNode", targetNodeName);
			modification = LDAPModification(attr, LDAPModification::OP_REPLACE);
			modlist->addModification(modification);
			attr = LDAPAttribute("sstMigrationSpicePort", targetSpicePort);
			modification = LDAPModification(attr, LDAPModification::OP_REPLACE);
			modlist->addModification(modification);
			SYSLOGLOGGER(logDEBUG) << (getName()) << ": change sstMigrationNode to " << targetNodeName;
			SYSLOGLOGGER(logDEBUG) << (getName()) << ": change sstMigrationSpicePort to " << targetSpicePort;
			lt->modifyEntry(getDn(), modlist);
			delete modlist;
		}

		vt->migrateVm(vm, targetNode, targetSpicePort);

		modlist = new LDAPModList();
		attr = LDAPAttribute("sstNode", targetNodeName);
		modification = LDAPModification(attr, LDAPModification::OP_REPLACE);
		modlist->addModification(modification);
		attr = LDAPAttribute("sstSpicePort", targetSpicePort);
		modification = LDAPModification(attr, LDAPModification::OP_REPLACE);
		modlist->addModification(modification);
		SYSLOGLOGGER(logDEBUG) << (getName()) << ": change sstNode to " << targetNodeName;
		SYSLOGLOGGER(logDEBUG) << (getName()) << ": change sstSpicePort to " << targetSpicePort;

		attr = LDAPAttribute("sstMigrationNode");
		modification = LDAPModification(attr, LDAPModification::OP_DELETE);
		modlist->addModification(modification);
		attr = LDAPAttribute("sstMigrationSpicePort");
		modification = LDAPModification(attr, LDAPModification::OP_DELETE);
		modlist->addModification(modification);
		SYSLOGLOGGER(logDEBUG) << (getName()) << ": remove sstMigrationNode";
		SYSLOGLOGGER(logDEBUG) << (getName()) << ": remove sstMigrationSpicePort";
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
	else if (string::npos != actDn.find(",ou=backup")) {
		if (0 == attr.compare("sstProvisioningMode")) {
			singleBackupCount++;
			if (0 != val.compare("finished")) {
				activeBackupMode = val;
				activeBackupDn = actDn;
			}
			else {
				finishedBackups.insert(actDn);
			}
		}
		else if (0 == attr.compare("sstProvisioningReturnValue")) {
			activeBackupReturnValue = atoi(val.c_str());
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

bool Vm::calculateBackupTime(time_t actTime) {
	bool retval = false;
	time_t backupTime = backupConfiguration.createTime();
	SYSLOGLOGGER(logDEBUG) << "Vm::calculateBackupTime: " << actTime << " <= " << backupTime << " && " << backupTime << " <= " << (actTime + Config::getInstance()->getCycle());
	if (0 < backupTime && actTime <= backupTime && backupTime <= actTime + Config::getInstance()->getCycle()) {
		retval = true;
	}
	return retval;
}

void Vm::handleBackupWorkflow(VirtTools* vt) {
	string newMode = "";
	string newState = "0";
	if (0 == activeBackupMode.compare("initialize")) {
		//20121002T010000Z
		string backupDn = "ou=backup,";
		backupDn.append(getDn());
		if (!lt->hasDn(backupDn)) {
			LDAPEntry* backupEntry = new LDAPEntry(backupDn);
			StringList values;
			values.add("top");
			values.add("organizationalUnit");

			backupEntry->addAttribute(LDAPAttribute("objectClass", values));
			backupEntry->addAttribute(LDAPAttribute("ou", "backup"));
			lt->addEntry(backupEntry);
			delete backupEntry;
		}
		char buffer[18];

		struct tm * timeinfo;
		time_t rawtime = backupConfiguration.getNextTime();
		timeinfo = localtime(&rawtime);

		strftime(buffer, 18, "%Y%m%dT%H%M00Z", timeinfo);

		activeBackupDn = "ou=";
		activeBackupDn.append(buffer).append(",").append(backupDn);
		if (!lt->hasDn(activeBackupDn)) {
			LDAPEntry* singelBackupEntry = new LDAPEntry(activeBackupDn);
			StringList values;
			values.add("top");
			values.add("organizationalUnit");
			values.add("sstProvisioning");

			time(&rawtime);
			timeinfo = localtime(&rawtime);
			strftime(buffer, 18, "%Y%m%dT%H%M00Z", timeinfo);

			singelBackupEntry->addAttribute(LDAPAttribute("objectClass", values));
			singelBackupEntry->addAttribute(LDAPAttribute("ou", buffer));
			singelBackupEntry->addAttribute(LDAPAttribute("sstProvisioningExecutionDate", "0"));
			singelBackupEntry->addAttribute(LDAPAttribute("sstProvisioningMode", "initialized"));
			singelBackupEntry->addAttribute(LDAPAttribute("sstProvisioningState", "0"));
			lt->addEntry(singelBackupEntry);
			delete singelBackupEntry;
		}
		SYSLOGLOGGER(logDEBUG) << (getDn()) << ": change sstProvisioningMode from initialize to initialized";

		newMode = "snapshot";
		newState = "0";
	}
	else if (0 == activeBackupMode.compare("initialized")) {
		// This attribute is written by the fc-brokerd and used internally by the fc-brokerd.

		newMode = "snapshot";
		newState = "0";
	}
	else if (0 == activeBackupMode.compare("snapshotted") && 0 == activeBackupReturnValue) {
		//  The attribute is changed by the Backup-Daemon from snapshotting to snapshotted when the snapshot process has finished.

		newMode = "merge";
		newState = "0";
	}
	else if (0 == activeBackupMode.compare("merged") && 0 == activeBackupReturnValue) {
		// The attribute is changed by the Backup-Daemon from merging to merged when the merge process has finished.

		newMode = "retain";
		newState = "0";
	}
	else if (0 == activeBackupMode.compare("retained") && 0 == activeBackupReturnValue) {
		// The attribute is changed by the Backup-Daemon from retaining to retained when the retain process has finished.

		newMode = "finished";
		time_t rawtime;
		struct tm * timeinfo;
		char buffer[18];

		time(&rawtime);
		timeinfo = localtime(&rawtime);

		strftime(buffer, 18, "%Y%m%dT%H%M00Z", timeinfo);
		newState = buffer;
	}
	else if (0 == activeBackupMode.compare("deleted") && 0 == activeBackupReturnValue) {
		lt->removeEntry(activeBackupDn, true);
	}
	else if (0 == activeBackupMode.compare("unretainedLargeFiles") && 0 == activeBackupReturnValue) {
		// The attribute is changed by the Backup-Daemon from unretaining to unretained when the unretain process has finished.

		newMode = "restore";
		newState = "0";

		vt->stopVmForRestore(this);
		// Merge ldif
	}
	else if (0 == activeBackupMode.compare("restored") && 0 == activeBackupReturnValue) {
		// The attribute is changed by the Backup-Daemon from restoring to restored when the restore process has finished.

		newMode = "finished";
		time_t rawtime;
		struct tm * timeinfo;
		char buffer[18];

		time(&rawtime);
		timeinfo = localtime(&rawtime);

		strftime(buffer, 18, "%Y%m%dT%H%M00Z", timeinfo);
		newState = buffer;
	}
	if (0 != newMode.length()) {
		LDAPModList* modlist = new LDAPModList();
		const string value = newMode;
		LDAPAttribute attr = LDAPAttribute("sstProvisioningMode", value);
		LDAPModification modification = LDAPModification(attr, LDAPModification::OP_REPLACE);
		modlist->addModification(modification);
		const string value2 = newState;
		attr = LDAPAttribute("sstProvisioningState", value2);
		modification = LDAPModification(attr, LDAPModification::OP_REPLACE);
		modlist->addModification(modification);
		SYSLOGLOGGER(logDEBUG) << (getDn()) << ": change sstProvisioningMode from " << activeBackupMode << " to " << value;
		lt->modifyEntry(activeBackupDn, modlist);
		delete modlist;
	}
	if (0 == newMode.compare("finished") && 0 == activeBackupMode.compare("retained") && singleBackupCount > backupConfiguration.getIterations()) {
		// check for deletion of an older backup
		if (0 < finishedBackups.size()) {
			string oldBackupDn = *(finishedBackups.begin());

			LDAPModList* modlist = new LDAPModList();
			LDAPAttribute attr = LDAPAttribute("sstProvisioningMode", "delete");
			LDAPModification modification = LDAPModification(attr, LDAPModification::OP_REPLACE);
			modlist->addModification(modification);
			attr = LDAPAttribute("sstProvisioningState", "0");
			modification = LDAPModification(attr, LDAPModification::OP_REPLACE);
			modlist->addModification(modification);
			SYSLOGLOGGER(logDEBUG) << (getDn()) << ": change sstProvisioningMode from finished to delete";
			lt->modifyEntry(oldBackupDn, modlist);
			delete modlist;
		}
	}
}

ostream& operator <<(ostream& s, const struct tm& tm);

enum others {O_NONE, O_DAY, O_HOUR, O_MINUTE};
enum params {P_NONE, P_DOW_OK};

time_t VmBackupConfiguration::createTime() {
	time_t retval = 0;
	SYSLOGLOGGER(logDEBUG) << "VmBackupConfiguration::createTime: " << *this;

	if (cronActive && 0 < cronDay.length() && 0 < cronDayOfWeek.length() && 0 < cronHour.length() &&
			0 < cronMinute.length() && 0 < cronMonth.length()) {
		if (0 != cronMinute.compare("*") && (0 == cronDay.compare("*") || 0 == cronDayOfWeek.compare("*"))) {
			time_t rawtime;
			struct tm * timelocal;
			struct tm timeinfo;
			time(&rawtime);
			timelocal = localtime(&rawtime);
			timelocal->tm_sec = 0;
			memcpy(&timeinfo, timelocal, sizeof(timeinfo));
			others setOthers = O_NONE;
			params setParams = P_NONE;

			ptime t = ptime_from_tm(timeinfo);
			SYSLOGLOGGER(logDEBUG) << "Orig:  " << timeinfo << endl;
			if (0 != cronMonth.compare("*")) {
				int month = atoi(cronMonth.c_str());
				if (month < timeinfo.tm_mon) {
					//t = t + years(1);
					t = t + months(12 - timeinfo.tm_mon);
				}
				else {
					t = t + months(month - timeinfo.tm_mon);
				}
				setOthers = O_DAY;
				timeinfo = to_tm(t);
				timeinfo.tm_mon = month;
				t = ptime_from_tm(timeinfo);
			}
			SYSLOGLOGGER(logDEBUG) << "Month: " << timeinfo << endl;
			if (O_NONE == setOthers) {
				if (0 != cronDay.compare("*")) {
					int day = atoi(cronDay.c_str());
					if (day < timeinfo.tm_mday) {
						t = t + months(1);
						setOthers = O_HOUR;
						timeinfo = to_tm(t);
					}
					timeinfo.tm_mday = day;
					t = ptime_from_tm(timeinfo);
				}
				else if (0 != cronDayOfWeek.compare("*")) {
					mktime(&timeinfo);
					SYSLOGLOGGER(logDEBUG) << "W Day:   " << timeinfo.tm_wday;
					SYSLOGLOGGER(logDEBUG) << "  DOW:   " << cronDayOfWeek;

					std::vector < std::string > daysofweek;
					boost::algorithm::split(daysofweek, cronDayOfWeek, boost::is_any_of(","), boost::algorithm::token_compress_off);
					int day;
					unsigned int i;
					for (i=0; i<daysofweek.size(); i++) {
						day = atoi(daysofweek[i].c_str());
						if (day > timeinfo.tm_wday) {
							break;
						}
					}
					if (i == daysofweek.size()) {
						day = atoi(daysofweek[0].c_str());
						if (day < timeinfo.tm_wday) {
							t = t + days(7 - timeinfo.tm_wday);
						}
						else {
							t = t + days(day - timeinfo.tm_wday);
						}
					}
					else {
						t = t + days(day - timeinfo.tm_wday);
						setParams = P_DOW_OK;
					}
					setOthers = O_HOUR;
					timeinfo = to_tm(t);
					SYSLOGLOGGER(logDEBUG) << "W Day:   " << timeinfo;
				}
			}
			if (O_NONE == setOthers) {
				if (0 != cronHour.compare("*")) {
					int hour = atoi(cronHour.c_str());
					if (hour < timeinfo.tm_hour) {
						if (0 != cronHour.compare("*")) {
							t = t + months(1);
						}
						else {
							t = t + days(7);
						}
						setOthers = O_MINUTE;

						timeinfo = to_tm(t);
					}
					timeinfo.tm_hour = hour;
					t = ptime_from_tm(timeinfo);
				}
				SYSLOGLOGGER(logDEBUG) << "  Hour:  " << timeinfo;
			}
			if (O_NONE == setOthers) {
				int minute = atoi(cronMinute.c_str());
				if (minute < timeinfo.tm_min) {
					t = t + hours(1);
					timeinfo = to_tm(t);

					//t = ptime_from_tm(timeinfo);
				}
				timeinfo.tm_min = minute;
				SYSLOGLOGGER(logDEBUG) << "  Min:   " << timeinfo;
				nextTime = mktime(&timeinfo);
			}

			if (O_NONE != setOthers) {
				switch (setOthers) {
					case O_DAY:
						if (0 != cronDay.compare("*")) {
							int day = atoi(cronDay.c_str());
							timeinfo.tm_mday = day;
						}
						else if (0 != cronDayOfWeek.compare("*")) {
							mktime(&timeinfo);
							int day = atoi(cronDayOfWeek.c_str());
							if (day < timeinfo.tm_wday) {
								t = t + days(7 - timeinfo.tm_wday);
							}
							else {
								t = t + days(day - timeinfo.tm_wday);
							}
							timeinfo = to_tm(t);
						}
						SYSLOGLOGGER(logDEBUG) << "O_Day:   " << timeinfo;
					case O_HOUR:
						if (0 != cronHour.compare("*")) {
							int hour = atoi(cronHour.c_str());
							if (hour < timeinfo.tm_hour) {
								if (0 != cronDay.compare("*")) {
									t = t + months(1);
								}
								else if (P_DOW_OK != setParams){
									t = t + days(7);
								}
								timeinfo = to_tm(t);
							}
							timeinfo.tm_hour = hour;
						}
						SYSLOGLOGGER(logDEBUG) << "O_Hour:  " << timeinfo;
					case O_MINUTE:
						int minute = atoi(cronMinute.c_str());
						timeinfo.tm_min = minute;
						SYSLOGLOGGER(logDEBUG) << "O_MIN:  " << timeinfo;
				}
			}
			retval = nextTime = mktime(&timeinfo);
		}
		else if (0 == cronMinute.compare("*")) {
			SYSLOGLOGGER(logINFO) << "Vm::createTime failed: sstCronMinute must not be '*'!";
		}
		else {
			SYSLOGLOGGER(logINFO) << "Vm::createTime failed: one of sstCronDay or sstCronDayOfWeek must be '*'!";
		}
	}
	else {
		SYSLOGLOGGER(logINFO) << "Vm::createTime failed: not all values set!";
	}
	return retval;
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
	s << std::endl << "+-> Backup: " << vm.activeBackupMode;
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
	s << vmDeviceInterface.name << " (" << vmDeviceInterface.type << ", " << vmDeviceInterface.modelType << "); "
			<< std::endl;
	s << "        +-> MacAdress: " << vmDeviceInterface.macAddress << std::endl;
	s << "        +-> SourceBridge: " << vmDeviceInterface.sourceBridge << std::endl;

	return s;
}

ostream& operator <<(ostream& s, const VmBackupConfiguration& vmBackupConfiguration) {
	s << "       Backup: excl: " << vmBackupConfiguration.exclude << ", it: " << vmBackupConfiguration.iterations;
	s << ";\tcronActive: " << vmBackupConfiguration.cronActive << "; cron: " << vmBackupConfiguration.cronMinute
			<< ", " << vmBackupConfiguration.cronHour << ", " << vmBackupConfiguration.cronDay << ", "
			<< vmBackupConfiguration.cronMonth << ", " << vmBackupConfiguration.cronDayOfWeek << std::endl;

	return s;
}

ostream& operator <<(ostream& s, const struct tm& tm) {
	s << tm.tm_mday << '.' << tm.tm_mon + 1 << '.'
			<< tm.tm_year + 1900 << " - " << tm.tm_hour
			<< ':' << tm.tm_min << " (" << tm.tm_wday << ")";
	return s;
}
