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

//#include "node.hpp"
//#include "vmPool.hpp"

class VmPool;
class Node;
class Vm;

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

	void addVm(Vm* vm);
	void removeVm(const Vm* vm);

	std::map<std::string, Vm*>* getVms() {
		return &vms;
	}

	std::map<std::string, Vm*>::const_iterator getVmsBegin() {
		return vms.begin();
	}

	std::map<std::string, Vm*>::const_iterator getVmsEnd() {
		return vms.end();
	}

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
private:
	int authCheck(const char *host, const char *port, const char *bind_dn, const char *bind_pw);

};

#endif /* CONFIG_HPP_ */
