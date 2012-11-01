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
 * vmPool.hpp
 *
 *  Created on: 11.05.2012
 *      Author: cwi
 */

#ifndef VMPOOL_HPP_
#define VMPOOL_HPP_

#include <map>
#include <set>
#include <string>

#include "ldapData.hpp"
#include "evenlyPolicy.hpp"
#include "networkRange.hpp"
#include "node.hpp"
#include "logger.hpp"


class LdapTools;
//class Node;
class Vm;
class VirTools;


class VmPoolNodeWrapper
{
	Node* node;				// owner is class Config
	std::set<Vm*> vms;		// owner is class Config
public:
	VmPoolNodeWrapper(Node* node_) : node(node_) {};
	virtual ~VmPoolNodeWrapper() {};

	const std::string& getName () const;

	void addVm(Vm* vm)
	{
		vms.insert(vm);
		node->addVm(vm);
	}
	void removeVm(Vm* vm)
	{
		vms.erase(vm);
		node->removeVm(vm);
	}

	std::set<Vm*>* getVms()
		{
			return &vms;
		}
	const Node* getNode () const
	{
		return node;
	}
	int getNumberVms() const
	{
		return static_cast<int>(vms.size());
	}

	int migrateFirstVm(VmPoolNodeWrapper* targetWrapper, VirtTools* vt);

	friend std::ostream& operator <<(std::ostream& s, const VmPoolNodeWrapper& nodeWrapper);
};

class VmPool : public LdapData {
private:
	std::string name;
	std::string displayName;
	std::string type;
	NetworkRange* range;
	std::string storagePoolName;
	std::string storagePoolDir;
	std::map<std::string, VmPoolNodeWrapper*> nodeWrappers;
	std::map<std::string, Vm*> vms;		// owner is class Config
	Vm* goldenImage;
	BasePolicy* policy;

public:
	VmPool(std::string dn_) : LdapData(dn_), range(NULL), goldenImage(NULL), policy(new EvenlyPolicy()) {}
	VmPool(std::string dn_, LdapTools* lt_) : LdapData(dn_, lt_), range(NULL), goldenImage(NULL), policy(new EvenlyPolicy()) {
	}
	virtual ~VmPool() {
		delete range;
		delete goldenImage;
		delete policy;
		std::map<std::string, VmPoolNodeWrapper*>::iterator itWrappers = nodeWrappers.begin();
		while (itWrappers != nodeWrappers.end()) {
			SYSLOGLOGGER(logDEBUG) << "VmPool::~VmPool: NodeWrappers; delete " << itWrappers->second->getName();
			delete itWrappers->second;
			//nodeWrappers.erase(itWrappers++);
			itWrappers++;
		}
		nodeWrappers.clear();
	}

	bool addAttribute(const std::string& actDn, const std::string& attr, const std::string& val);
	bool hasPolicy();
	bool hasActiveGoldenImage();

	void setVms(std::map<std::string, Vm*> vms);
	void addVm(Vm* vm);
	void removeVm(Vm* vm);

	void addNode(Node* node);

	const std::string& getName() const {
		return name;
	}
	const std::string& getDisplayName() const {
		return displayName;
	}
	const std::string& getType() const {
		return type;
	}
	const NetworkRange* getRange() const {
		return range;
	}
	const std::string& getStoragePoolName() const {
		return storagePoolName;
	}
	const std::string& getStoragePoolDir() const {
		return storagePoolDir;
	}
	const std::map<std::string, VmPoolNodeWrapper*>* getNodeWrappers() const {
		return &nodeWrappers;
	}
	const VmPoolNodeWrapper* getNodeWrapper(const std::string& nodeName) const {
		VmPoolNodeWrapper* retval = NULL;
		std::map<std::string, VmPoolNodeWrapper*>::const_iterator it = nodeWrappers.find(nodeName);
		if (nodeWrappers.end() != it) {
			retval = it->second;
		}
		return retval;
	}
	const std::map<std::string, Vm*>* getVms() const {
		return &vms;
	}
	const Vm* getGoldenImage() const {
		return goldenImage;
	}
	const BasePolicy* getPolicy() const {
		return policy;
	}
	void setPolicy(BasePolicy* policy_)
	{
		if (policy)
		{
			delete policy;
		}
		policy = policy_;
	}

	friend std::ostream& operator <<(std::ostream& s, const VmPool& vmPool);
};

#endif /* VMPOOL_HPP_ */
