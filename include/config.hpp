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
 * config.hpp
 *
 *  Created on: 11.05.2012
 *      Author: cwi
 */

#ifndef CONFIG_HPP_
#define CONFIG_HPP_

#include <map>
#include <string>

#include "ldap.h"

//#include "vmPool.hpp"

class VmPool;
class Node;
class Vm;
class VmBackupConfiguration;

class Config {
private:
	static Config* instance;
	Config();
	Config(const Config&);
	Config& operator=(const Config&);
	~Config();

	LDAP *ld;
	std::string ldapuri;
	std::string ldapbinddn;
	std::string ldapbindpwd;
	std::string ldapbasedn;
	int cycle;

	std::map<std::string, VmPool*> vmPools;
	std::map<std::string, Node*> nodes;
	std::map<std::string, Vm*> vms;

	/* configuration settings */
	bool allowSound;
	bool allowSpice;
	int spicePortMin;
	int spicePortMax;
	bool allowUsb;

	/* for backup */
	VmBackupConfiguration* globalBackupConfiguration;
	std::map<std::string, Vm*> backupVms;

	/* for shutdown */
	std::map<std::string, VmPool*> shutdownVmPools;

public:
	static std::string filename;

	static Config* getInstance();

	void clearMaps();

	void addVmPool(VmPool* vmPool);

	std::map<std::string, VmPool*>* getVmPools() {
		return &vmPools;
	}
	const VmPool* getVmPoolByName(const std::string& poolName) {
		VmPool* retval = NULL;
		std::map<std::string, VmPool*>::iterator it = vmPools.find(poolName);
		if (vmPools.end() != it) {
			retval = it->second;
		}
		return retval;
	}

	void addNode(Node* node);

	std::map<std::string, Node*>* getNodes() {
		return &nodes;
	}
	const Node* getNodeByName(const std::string& nodeName) {
		Node* retval = NULL;
		std::map<std::string, Node*>::iterator it = nodes.find(nodeName);
		if (nodes.end() != it) {
			retval = it->second;
		}
		return retval;
	}

	std::map<std::string, Vm*>* getBackupVms() {
		return &backupVms;
	}

	void addVm(Vm* vm);
	void removeVm(const Vm* vm);

	void handleVmForBackup(Vm* vm, time_t nextTime);

	std::map<std::string, Vm*>* getVms() {
		return &vms;
	}

	std::map<std::string, Vm*>::const_iterator getVmsBegin() {
		return vms.begin();
	}

	std::map<std::string, Vm*>::const_iterator getVmsEnd() {
		return vms.end();
	}

	VmBackupConfiguration* getGlobalBackupConfiguration() const {
		return globalBackupConfiguration;
	}

	std::map<std::string, VmPool*>* getShutdownVmPools() {
		return &shutdownVmPools;
	}
	void addShutdownVmPool(VmPool* vmPool);

    const std::string& getLdapBindDn() const
    {
        return ldapbinddn;
    }

    const std::string& getLdapBindPwd() const
    {
        return ldapbindpwd;
    }

    const std::string& getLdapUri() const
    {
        return ldapuri;
    }

    const std::string& getLdapBaseDn() const
    {
        return ldapbasedn;
    }

    const int getCycle() const
    {
        return cycle;
    }

	const bool isSoundAllowed() const {
		return allowSound;
	}
	const bool isSpiceAllowed() const {
		return allowSpice;
	}
	const int getSpicePortMin() const
    {
        return spicePortMin;
    }
	const int getSpicePortMax() const
    {
        return spicePortMax;
    }
	const bool isUsbAllowed() const {
		return allowUsb;
	}

	void setAllowSound(bool on) {
		allowSound = on;
	}
	void setAllowSpice(bool on) {
		allowSpice = on;
	}
	void setSpicePortMin(int min) {
		spicePortMin = min;
	}
	void setSpicePortMax(int max) {
		spicePortMax = max;
	}
	void setAllowUsb(bool on) {
		allowUsb = on;
	}

private:
	int authCheck(const char *host, const char *port, const char *bind_dn, const char *bind_pw);

};

#endif /* CONFIG_HPP_ */
