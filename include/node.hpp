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
 * node.hpp
 *
 *  Created on: 11.05.2012
 *      Author: cwi
 */

#ifndef NODE_HPP_
#define NODE_HPP_

#include <map>
#include <string>
#include <iostream>
#include <set>

#include "ldapData.hpp"
#include "vm.hpp"
#include "logger.hpp"


class NodeType {
private:
	std::string type;
	std::string subType;
	std::string state;

public:
	NodeType() {};
	virtual ~NodeType() {};

	void setType(const std::string type_) {
		this->type = type_;
	}
	const std::string& getType() const {
		return type;
	}
	void setSubType(const std::string subType_) {
		this->subType = subType_;
	}
	const std::string& getSubType() const {
		return subType;
	}
	void setState(const std::string state_) {
		this->state = state_;
	}
	const std::string& getState() const {
		return state;
	}
};

class NodeNetwork {
private:
	std::string name;
	std::string ip;
	// for later use
	// std::vector<std::string> services;

public:
	NodeNetwork() {};
	virtual ~NodeNetwork() {};
    const std::string getIP() const
    {
        return ip;
    }

    const std::string getName() const
    {
        return name;
    }

    void setIP(const std::string ip_)
    {
        this->ip = ip_;
    }

    void setName(const std::string name_)
    {
        this->name = name_;
    }
};

class Node : public LdapData {
private:
	std::string name;
	std::map<std::string, NodeType*> types;
	std::map<std::string, NodeNetwork*> networks;
	std::set<Vm*> vms;
	bool maintenance;

public:
	Node(std::string dn_, std::string name_ = std::string("")) :
			LdapData(dn_), name(name_), maintenance(false) {};
	Node(std::string dn_, LdapTools* lt_, std::string name_ = std::string("")) :
			LdapData(dn_, lt_), name(name_), maintenance(false) {};
	virtual ~Node() {
		std::map<std::string, NodeType*>::iterator itTypes = types.begin();
		while (itTypes != types.end()) {
			SYSLOGLOGGER(logDEBUG) << "Node::~Node: Types; delete " << itTypes->second->getType();
			delete itTypes->second;
			//types.erase(itTypes++);
			itTypes++;
		}
		types.clear();
		std::map<std::string, NodeNetwork*>::iterator itNetworks = networks.begin();
		while (itNetworks != networks.end()) {
			SYSLOGLOGGER(logDEBUG) << "Node::~Node: Networks; delete " << itNetworks->second->getName();
			delete itNetworks->second;
			//networks.erase(itNetworks++);
			itNetworks++;
		}
		networks.clear();
	};

	bool addAttribute(const std::string& actDn, const std::string& attr, const std::string& val);

	bool hasType(const std::string type);

	NodeType* getType(const std::string type);

	const std::string& getName() const {
		return name;
	}

	void addVm(Vm* vm)
	{
		if (vms.find(vm) == vms.end()) {
			vms.insert(vm);
		}
	}
	void removeVm(Vm* vm)
	{
		vms.erase(vm);
	}

	std::set<Vm*>* getVms()
	{
		return &vms;
	}
//	const int getNumberVms() const {
//		return static_cast<int>(vms.size());
//	}

	const std::string getVirtUri() const {
		std::string uri = "qemu+tcp://";
		uri.append(getVLanIP("int")).append("/system");
		return uri;
	}

	const std::string getVLanIP(std::string type) const {
		std::map<std::string, NodeNetwork*>::const_iterator it = networks.find(type);
		if (it != networks.end()) {
			NodeNetwork* network = (*it).second;
			return network->getIP();
		}
		return "???";
	}

	const bool isMaintenance() const {
		return maintenance;
	}
	void setMaintenance(const bool maintenance_) {
		maintenance = maintenance_;
	}

	void logging() const;

	friend std::ostream& operator <<(std::ostream& s, const Node& node);
};

#endif /* NODE_HPP_ */
