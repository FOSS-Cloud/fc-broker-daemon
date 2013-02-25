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
 * node.cpp
 *
 *  Created on: 11.05.2012
 *      Author: cwi
 */

#include "include/node.hpp"
#include "include/logger.hpp"

extern "C" {
#include "stdlib.h"
}

using namespace std;

bool Node::addAttribute(const string& actDn, const string& attr, const string& val) {
	if (0 == actDn.compare(this->dn)) {
		//SYSLOGLOGGER(logDEBUG) << "addAttribute: Node " << attr;
		if (0 == attr.compare("sstNode")) {
			name = string(val);
		}
	}
	else if (/*string::npos != actDn.find("node-types") &&*/ string::npos != actDn.find("sstNodeType")) {
		//SYSLOGLOGGER(logDEBUG)  << "addAttribute: NodeType " << attr;
		string type;
		getDnPart(actDn, "sstNodeType", type);
		map<string, NodeType*>::iterator it = types.find(type);
		NodeType* nodeType = NULL;
		if (types.end() != it) {
			nodeType = it->second;
		}
		else {
			nodeType = new NodeType();
			types[type] = nodeType;
		}
		if (0 == attr.compare("sstNodeType")) {
			nodeType->setType(val);
		}
		else if (0 == attr.compare("sstNodeSubType")) {
			nodeType->setSubType(val);
		}
	}
	else if (/*string::npos != actDn.find("node-types") &&*/ string::npos != actDn.find("networks")) {
		if (string::npos != actDn.find("ou=int")) {
			//SYSLOGLOGGER(logDEBUG)  << "addAttribute: NodeNetwork " << attr;
			string type;
			getDnPart(actDn, "ou", type);
			map<string, NodeNetwork*>::iterator it = networks.find(type);
			NodeNetwork* nodeNetwork = NULL;
			if (networks.end() != it) {
				nodeNetwork = it->second;
			}
			else {
				nodeNetwork = new NodeNetwork();
				networks[type] = nodeNetwork;
			}
			if (0 == attr.compare("ou")) {
				nodeNetwork->setName(val);
			}
			else if (0 == attr.compare("sstNetworkIPAddress")) {
				nodeNetwork->setIP(val);
			}
		}
		else if (string::npos != actDn.find("ou=pub")) {
			//SYSLOGLOGGER(logDEBUG)  << "addAttribute: NodeNetwork " << attr;
			string type;
			getDnPart(actDn, "ou", type);
			map<string, NodeNetwork*>::iterator it = networks.find(type);
			NodeNetwork* nodeNetwork = NULL;
			if (networks.end() != it) {
				nodeNetwork = it->second;
			}
			else {
				nodeNetwork = new NodeNetwork();
				networks[type] = nodeNetwork;
			}
			if (0 == attr.compare("ou")) {
				nodeNetwork->setName(val);
			}
			else if (0 == attr.compare("sstNetworkIPAddress")) {
				nodeNetwork->setIP(val);
			}
		}
	}
	return true;
}

bool Node::hasType(std::string& checkType) {
	bool retval = false;
	map<string, NodeType*>::const_iterator it;
	for (it = types.begin(); it != types.end() && !retval; it++) {
		string type = it->second->getType();
		retval = 0 == type.compare(checkType);
	}
	return retval;
}

void Node::logging() const {
	SYSLOGLOGGER(logINFO) << name << "; ";
	string tmp = "";
/*
	for (map<string, NodeType*>::const_iterator it=types.begin(); it != types.end(); it++) {
    	NodeType* type = (*it).second;
    	tmp.append(type->getType()).append(" (").append(type->getSubType()).append("); ");
    }
    SYSLOGLOGGER(logINFO) << "+-> " << tmp;
    tmp = "";
    for (map<string, NodeNetwork*>::const_iterator it=networks.begin(); it != networks.end(); it++) {
    	NodeNetwork* network = (*it).second;
    	tmp.append(network->getName()).append(" (").append(network->getIP()).append("); ");
    }
    SYSLOGLOGGER(logINFO) << "+-> " << tmp;
*/
    SYSLOGLOGGER(logINFO) << "+-> URI: " << getVirtUri();
    SYSLOGLOGGER(logINFO) << "+-> Vms:";
    for (set<Vm*>::const_iterator itVms = vms.begin(); itVms != vms.end(); itVms++) {
    	Vm* vm = *itVms;
    	tmp = "    ";
    	tmp.append(vm->getName()).append(" (").append(vm->getDisplayName()).append(") ");
		switch(vm->getStatus()) {
			case VmCheckAgain: tmp.append("CHECK AGAIN"); break;
			case VmRunning: tmp.append("RUNNING"); break;
			default: tmp.append("UNKNOWN"); break;
		}
		if (0 < vm->isAssignedToUser()) {
			tmp.append(" for user ").append(vm->getUser());
		}
		else if (vm->getStatus() == VmRunning) {
			tmp.append(" prestarted");
		}
		SYSLOGLOGGER(logINFO) << tmp;
    }
}

ostream& operator << (ostream& s, const Node& node){
    s << node.name << "; ";
    s << std::endl << "+-> ";
    for (map<string, NodeType*>::const_iterator it=node.types.begin(); it != node.types.end(); it++) {
    	NodeType* type = (*it).second;
    	s << type->getType() << " (" << type->getSubType() << "); ";
    }
    s << std::endl << "+-> ";
    for (map<string, NodeNetwork*>::const_iterator it=node.networks.begin(); it != node.networks.end(); it++) {
    	NodeNetwork* network = (*it).second;
    	s << network->getName() << " (" << network->getIP() << "); ";
    }
    s << std::endl << "+-> URI: " << node.getVirtUri();
    s << std::endl << "+-> Vms:" << std::endl;
    for (set<Vm*>::const_iterator itVms = node.vms.begin(); itVms != node.vms.end(); itVms++) {
    	Vm* vm = *itVms;
    	s << "      " << vm->getName() << " (" << vm->getDisplayName() << ") ";
		switch(vm->getStatus()) {
			case VmCheckAgain: s << "CHECK AGAIN"; break;
			case VmRunning: s << "RUNNING"; break;
			default: s << "UNKNOWN"; break;
		}
		s << endl;
    }

	return s;
}
