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
 * config.cpp
 *
 *  Created on: 11.05.2012
 *      Author: cwi
 */

#include <iostream>
#include <fstream>
#include <vector>

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/ini_parser.hpp"

extern "C" {
#include "stdlib.h"
#include "ldap.h"
}

#include "include/config.hpp"
#include "include/vmPool.hpp"
#include "include/node.hpp"
#include "include/vm.hpp"
#include "include/logger.hpp"

using boost::property_tree::ptree;

using namespace std;

Config* Config::instance = NULL;
string Config::filename = "";

/**
 * get THE instance of Config
 * Singleton Design Pattern
 *
 * @return Config*
 */
Config* Config::getInstance() {
	if (NULL == instance) {
		instance = new Config();
	}
	return instance;
}

/**
 *	Constructor
 */
Config::Config() {
	ptree pt;
	read_ini(filename, pt);

	this->ldapuri = string(pt.get<std::string>("ldap.uri"));
	this->ldapbinddn = string(pt.get<std::string>("ldap.binddn"));
	this->ldapbindpwd = string(pt.get<std::string>("ldap.bindpwd"));
	this->ldapbasedn = string(pt.get<std::string>("ldap.basedn"));
	this->cycle = pt.get("general.cycle", 30);

	this->ld = NULL;
	this->globalBackupConfiguration = new VmBackupConfiguration();
	this->allowSound = false;
	this->allowUsb = false;
	this->allowSpice = false;
}

Config::~Config() {
	clearMaps();
	delete globalBackupConfiguration;
}

void Config::clearMaps() {
	map<string, Vm*>::iterator itVms = vms.begin();
	while (itVms != vms.end()) {
		SYSLOGLOGGER(logDEBUG) << "Config::clearMaps: Vms; delete " << itVms->second->getName();
		delete itVms->second;
		//vms.erase(itVms++);
		itVms++;
	}
	vms.clear();
	map<string, Node*>::iterator itNodes = nodes.begin();
	while (itNodes != nodes.end()) {
		SYSLOGLOGGER(logDEBUG) << "Config::clearMaps: Nodes; delete " << itNodes->second->getName();
		delete itNodes->second;
		//nodes.erase(itNodes++);
		itNodes++;
	}
	nodes.clear();
	map<string, VmPool*>::iterator itVmPools = vmPools.begin();
	while (itVmPools != vmPools.end()) {
		SYSLOGLOGGER(logDEBUG) << "Config::clearMaps: VmPools; clear " << itVmPools->second->getName();
		itVmPools->second->clear();
		//delete itVmPools->second;
		itVmPools++;
	}
	//vmPools.clear();
	map<string, Vm*>::iterator itBackupVms = backupVms.begin();
	while (itBackupVms != backupVms.end()) {
		SYSLOGLOGGER(logDEBUG) << "Config::clearMaps: BackupVms; delete " << itBackupVms->second->getName();
		delete itBackupVms->second;
		itBackupVms++;
	}
	backupVms.clear();

//	map<string, VmPool*>::iterator itShutdownVmPools = shutdownVmPools.begin();
//	while (itShutdownVmPools != shutdownVmPools.end()) {
//		SYSLOGLOGGER(logDEBUG) << "Config::clearMaps: ShutdownVmPools; delete " << itShutdownVmPools->second->getName();
//		delete itShutdownVmPools->second;
//		itShutdownVmPools++;
//	}
	shutdownVmPools.clear();
}

void Config::addVmPool(VmPool* vmPool) {
	vmPools[vmPool->getName()] = vmPool;
}

void Config::addNode(Node* node) {
	SYSLOGLOGGER(logDEBUG) << "Config::addNode: " << node->getName();
	nodes[node->getName()] = node;
}

void Config::addVm(Vm* vm) {
	SYSLOGLOGGER(logDEBUG) << "Config::addVm: " << vm->getName();
	vms[vm->getName()] = vm;
}

void Config::removeVm(const Vm* vm) {
	SYSLOGLOGGER(logDEBUG) << "Config::removeVm: " << vm->getName();
	vms.erase(vm->getName());
}

void Config::handleVmForBackup(Vm* vm, time_t nextTime) {
	SYSLOGLOGGER(logDEBUG) << "Config::handleVmForBackup: " << vm->getName();
	if (0 == vm->getActiveBackupMode().compare("unknown")) {
		vm->setActiveBackupMode("initialize");
	}
	SYSLOGLOGGER(logDEBUG) << "  " << vm->getActiveBackupMode();

	if ((0 == vm->getActiveBackupMode().compare("initialize") && vm->calculateBackupTime(nextTime)) ||
			0 != vm->getActiveBackupDn().length()) {
		SYSLOGLOGGER(logDEBUG) << "   added";
		backupVms[vm->getActiveBackupDn()] = vm;
	}
}

void Config::addShutdownVmPool(VmPool* vmPool) {
	SYSLOGLOGGER(logDEBUG) << "Config::addShutdownVmPool: " << vmPool->getName();
	shutdownVmPools[vmPool->getName()] = vmPool;
}

