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
 * vm.hpp
 *
 *  Created on: 11.05.2012
 *      Author: cwi
 */

#ifndef VM_HPP_
#define VM_HPP_

#include <map>
#include <ctime>

#include "StringList.h"

#include "config.hpp"
#include "ldapData.hpp"
#include "logger.hpp"

class VmDeviceDisk;
class VmDeviceInterface;
class Node;
class VirtTools;

enum VmStatus {VmStatusUnknown = 0, VmCheckAgain, VmRunning };

/*
objectclass ( sstObjectClass:28
    NAME 'sstVirtualizationVirtualMachineDisk'
    SUP top STRUCTURAL
    MUST ( sstDevice $ sstDisk $ sstSourceFile $ sstTargetBus $ sstType )
    MAY ( sstDriverCache $ sstDriverName $ sstDriverType $ sstReadonly $ sstVolumeAllocation $
          sstVolumeCapacity $ sstVolumeName) )

 */
class VmDeviceDisk {
private:
	std::string name; /* is sstDisk */
	std::string type;
	std::string device;
	std::string sourceFile;
	std::string targetBus;
	std::string driverCache;
	std::string driverName;
	std::string driverType;
	bool readonly;
	std::string volumeName;
	unsigned long volumeCapacity;

public:
	VmDeviceDisk() : driverCache(""), driverName(""), driverType(""), readonly(false) {};
	virtual ~VmDeviceDisk() {};

	const std::string& getName() const {
		return name;
	}

	const std::string& getType() const {
		return type;
	}

	const std::string& getDevice() const {
		return device;
	}

	const std::string& getSourceFile() const {
		return sourceFile;
	}

	const std::string& getTargetBus() const {
		return targetBus;
	}

	const std::string& getDriverCache() const {
		return driverCache;
	}

	const std::string& getDriverName() const {
		return driverName;
	}

	const std::string& getDriverType() const {
		return driverType;
	}

	const bool isReadonly() const {
		return readonly;
	}

	const std::string& getVolumeName() const {
		return volumeName;
	}

	const unsigned long getVolumeCapacity() const {
		return volumeCapacity;
	}

	void setName(const std::string name_) {
		this->name = name_;
	}

	void setType(const std::string type_) {
		this->type = type_;
	}

	void setDevice(const std::string device_) {
		this->device = device_;
	}

	void setSourceFile(const std::string sourceFile_) {
		this->sourceFile = sourceFile_;
	}

	void setTargetBus(const std::string targetBus_) {
		this->targetBus = targetBus_;
	}

	void setDriverCache(const std::string driverCache_) {
		this->driverCache = driverCache_;
	}

	void setDriverName(const std::string driverName_) {
		this->driverName = driverName_;
	}

	void setDriverType(const std::string driverType_) {
		this->driverType = driverType_;
	}

	void setReadonly(const bool readonly_) {
		this->readonly = readonly_;
	}

	void setVolumeName(const std::string volumeName_) {
		this->volumeName = volumeName_;
	}

	void setVolumeCapacity(unsigned long volumeCapacity_) {
		this->volumeCapacity = volumeCapacity_;
	}

	friend std::ostream& operator <<(std::ostream& s, const VmDeviceDisk& vmDeviceDisk);
};

/*
objectclass ( sstObjectClass:29
    NAME 'sstVirtualizationVirtualMachineInterface'
    SUP top STRUCTURAL
    MUST ( sstInterface $ sstMacAddress $ sstModelType $ sstSourceBridge $ sstType ) )
*/
class VmDeviceInterface {
private:
	std::string name;
	std::string type;
	std::string macAddress;
	std::string modelType;
	std::string sourceBridge;

public:
	VmDeviceInterface() {};
	virtual ~VmDeviceInterface() {};

	const std::string& getName() const {
		return name;
	}
	const std::string& getType() const {
		return type;
	}
	const std::string& getMacAddress() const {
		return macAddress;
	}
	const std::string& getModelType() const {
		return modelType;
	}
	const std::string& getSourceBridge() const {
		return sourceBridge;
	}

	void setName(const std::string name_) {
		this->name = name_;
	}

	void setType(const std::string type_) {
		this->type = type_;
	}

	void setMacAddress(const std::string macAddress_) {
		this->macAddress = macAddress_;
	}

	void setModelType(const std::string modelType_) {
		this->modelType = modelType_;
	}

	void setSourceBridge(const std::string sourceBridge_) {
		this->sourceBridge = sourceBridge_;
	}

	friend std::ostream& operator <<(std::ostream& s, const VmDeviceInterface& vmDeviceInterface);
};

/*
objectclass ( sstObjectClass:58
    NAME 'sstVirtualizationBackupObjectClass'
    SUP top AUXILIARY
    MAY ( sstBackupNumberOfIterations $ sstBackupRootDirectory $ sstBackupRetainDirectory $
          sstVirtualizationVirtualMachineForceStart $ sstVirtualizationBandwidthMerge $
          sstRestoreVMWithoutState $ sstBackupExcludeFromBackup $ sstBackupRamDiskLocation $
          sstVirtualizationVirtualMachineSequenceStop $ sstVirtualizationVirtualMachineSequenceStart ) )

objectclass ( sstObjectClass:57
    NAME 'sstCronObjectClass'
    SUP top AUXILIARY
    MUST ( sstCronMinute $ sstCronHour $ sstCronDay $ sstCronMonth $ sstCronDayOfWeek $ sstCronActive )
    MAY ( sstCronCommand ) )
*/
class VmBackupConfiguration {
private:
	bool set;
	int iterations;
	bool exclude;
	bool cronActive;
	std::string cronDay;
	std::string cronDayOfWeek;
	std::string cronHour;
	std::string cronMinute;
	std::string cronMonth;
	time_t nextTime;

public:
	VmBackupConfiguration() {
		set = false;
		iterations = 0;
		exclude = false;
		cronActive = false;
		nextTime = 0;
	};
	virtual ~VmBackupConfiguration() {};

	time_t createTime();

	time_t getNextTime() {
		return nextTime;
	}

	const int getIterations() const {
		return iterations;
	}

	const bool isExcluded() const {
		return exclude;
	}

	void setIterations(const int iterations_) {
		this->iterations = iterations_;
		this->set = true;
	}

	void setExclude(const bool exclude_) {
		this->exclude = exclude_;
		this->set = true;
	}

	void setCronActive(const bool active) {
		this->cronActive = active;
		this->set = true;
	}

	void setCronDay(const std::string day) {
		this->cronDay = day;
		this->set = true;
	}

	void setCronDayOfWeek(const std::string day) {
		this->cronDayOfWeek = day;
		this->set = true;
	}

	void setCronHour(const std::string hour) {
		this->cronHour = hour;
		this->set = true;
	}

	void setCronMinute(const std::string minute) {
		this->cronMinute = minute;
		this->set = true;
	}

	void setCronMonth(const std::string month) {
		this->cronMonth = month;
		this->set = true;
	}

	const bool isSet() const {
		return set;
	}

	friend std::ostream& operator <<(std::ostream& s, const VmBackupConfiguration& vmBackupConfiguration);
};

class Vm : public LdapData {
private:
	std::string name;
	std::string displayName;
	std::string nodeName;
	std::string vmPoolName;
	std::string user;
	std::map<std::string, VmDeviceDisk*> disks;
	std::map<std::string, VmDeviceInterface*> interfaces;
	std::string vmType;
	std::string vmSubType;

	std::string memory;
	std::string clockOffset;
	std::string onCrash;
	std::string onPowerOff;
	std::string onReboot;
	std::string osArchitecture;
	std::string osBootDevice;
	std::string osMachine;
	std::string osType;
	std::string type;
	std::string vCpu;
	StringList features;
	std::string spicePort;
	std::string spicePassword;
	std::string emulator;
	std::string memBalloon;

	VmStatus status;

	/* for backup */
	VmBackupConfiguration backupConfiguration;
	std::string activeBackupDn;
	std::string activeBackupMode;
	int activeBackupReturnValue;
	int singleBackupCount;

public:
	Vm(std::string dn_, std::string name_ = std::string("")) :
			LdapData(dn_), name(name_), status(VmStatusUnknown), activeBackupDn(""), activeBackupMode("unknown"), activeBackupReturnValue(0), singleBackupCount(0) {
	}
	Vm(std::string dn_, LdapTools* lt_, std::string name_ = std::string("")) :
			LdapData(dn_, lt_), name(name_), status(VmStatusUnknown), activeBackupDn(""), activeBackupMode("unknown"), activeBackupReturnValue(0), singleBackupCount(0) {
	}
	virtual ~Vm() {
		std::map<std::string, VmDeviceDisk*>::iterator itDisks = disks.begin();
		while (itDisks != disks.end()) {
			SYSLOGLOGGER(logDEBUG) << "Vm::~Vm: Disks; delete " << itDisks->second->getName();
			delete itDisks->second;
			//disks.erase(itDisks++);
			itDisks++;
		}
		disks.clear();
		std::map<std::string, VmDeviceInterface*>::iterator itInterfaces = interfaces.begin();
		while (itInterfaces != interfaces.end()) {
			SYSLOGLOGGER(logDEBUG) << "Vm::~Vm: Interfaces: delete " << itInterfaces->second->getName();
			delete itInterfaces->second;
			//interfaces.erase(itInterfaces++);
			itInterfaces++;
		}
		interfaces.clear();
	}

	void remove();

	void migrate(const Node* node, VirtTools* vt);

	bool addAttribute(const std::string& actDn, const std::string& attr, const std::string& val);

	const VmDeviceDisk* getDiskByDeviceName(const std::string& name) const;

	const std::map<std::string, VmDeviceDisk*>* getDisks() const {
		return &disks;
	}
	const std::map<std::string, VmDeviceInterface*>* getInterfaces() const {
		return &interfaces;
	}

	const std::string& getName() const {
		return name;
	}

	const std::string& getDisplayName() const {
		return displayName;
	}

	const std::string& getNodeName() const {
		return nodeName;
	}
	const Node* getNode() const {
		return Config::getInstance()->getNodeByName(nodeName);
	}
	const std::string& getVmPoolName() const {
		return vmPoolName;
	}
	const VmPool* getVmPool() const {
		return Config::getInstance()->getVmPoolByName(vmPoolName);
	}
	const VmStatus getStatus() const {
		return status;
	}
	const std::string& getUser() const {
		return user;
	}
	const bool isAssignedToUser() const {
		return !user.empty();
	}
	const std::string& getVmType() const {
		return vmType;
	}
	const std::string& getVmSubType() const {
		return vmSubType;
	}
	const bool isGoldenImage() const {
		return 0 == vmSubType.compare("Golden-Image");
	}
	const bool isDynVm() const {
		return 0 == vmType.compare("dynamic");
	}
	const std::string& getMemory() const {
		return memory;
	}
	const long getMemoryKb() const {
		return atol(memory.c_str()) / 1024;
	}
	const std::string& getClockOffset() const {
		return clockOffset;
	}
	const std::string& getOnCrash() const {
		return onCrash;
	}
	const std::string& getOnPowerOff() const {
		return onPowerOff;
	}
	const std::string& getOnReboot() const {
		return onReboot;
	}
	const std::string& getOsArchitecture() const {
		return osArchitecture;
	}
	const std::string& getOsBootDevice() const {
		return osBootDevice;
	}
	const std::string& getOsMachine() const {
		return osMachine;
	}
	const std::string& getType() const {
		return type;
	}
	const std::string& getOsType() const {
		return osType;
	}
	const std::string& getVCpu() const {
		return vCpu;
	}
	const StringList& getFeatures() const {
		return features;
	}
	const std::string& getSpicePort() const {
		return spicePort;
	}
	const std::string& getSpicePassword() const {
		return spicePassword;
	}
	const std::string& getEmulator() const {
		return emulator;
	}
	const std::string& getMemBalloon() const {
		return memBalloon;
	}

	void setName(std::string name_)
	{
		name = name_;
	}
	void setNodeName(std::string nodeName_)
	{
		nodeName = nodeName_;
	}
	void setUser(std::string user_)
	{
		user = user_;
	}
	void setStatus(const VmStatus status_) {
		this->status = status_;
	}
	void setFeatures(StringList features_) {
		this->features = features_;
	}

	/* for backup */
	void handleBackupWorkflow();

	const bool hasOwnBackupConfiguration() {
		return backupConfiguration.isSet();
	}
	VmBackupConfiguration* getBackupConfiguration() {
		return &backupConfiguration;
	}
	void setBackupConfiguration(const VmBackupConfiguration* backupConfig) {
		backupConfiguration = *backupConfig;
	}
	bool calculateBackupTime(time_t actTime);

	time_t getNextBackupTime() {
		return backupConfiguration.getNextTime();
	}
	const std::string& getActiveBackupDn() const {
		return activeBackupDn;
	}
	const std::string& getActiveBackupMode() const {
		return activeBackupMode;
	}
	const int getSingleBackupCount() const {
		return singleBackupCount;
	}
	bool isBackupNeeded() {
		bool retval = !getBackupConfiguration()->isExcluded();
		SYSLOGLOGGER(logDEBUG) << "Vm::isBackupNeeded: excluded " << getBackupConfiguration()->isExcluded();
		SYSLOGLOGGER(logDEBUG) << "Vm::isBackupNeeded: activeBackupDn " << activeBackupDn;
		SYSLOGLOGGER(logDEBUG) << "Vm::isBackupNeeded: count " << singleBackupCount << " < " << (getBackupConfiguration()->getIterations() + 1) << " " << (singleBackupCount < (getBackupConfiguration()->getIterations() + 1));
		retval = retval && (0 < activeBackupDn.length() || singleBackupCount < (getBackupConfiguration()->getIterations() + 1));
		SYSLOGLOGGER(logDEBUG) << "Vm::isBackupNeeded: " << retval;
		return retval;
	}

	void setActiveBackupMode(const std::string activeBackupMode_) {
		activeBackupMode = activeBackupMode_;
	}

	friend std::ostream& operator <<(std::ostream& s, const Vm& vm);
};

#endif /* VM_HPP_ */
