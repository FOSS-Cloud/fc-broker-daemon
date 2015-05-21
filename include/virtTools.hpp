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
 * virtTools.h
 *
 *  Created on: 31.05.2012
 *      Author: cwi
 */

#ifndef VIRTTOOLS_H_
#define VIRTTOOLS_H_

#include "boost/uuid/uuid.hpp"
#include "boost/uuid/random_generator.hpp"
#include "boost/uuid/uuid_io.hpp"
#include "boost/random/random_device.hpp"
#include "boost/random/uniform_int_distribution.hpp"

#include <string>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
//#include <ctime>
#include <sys/time.h>
#include <map>

#include "config.hpp"

extern "C" {
#include "libvirt/libvirt.h"
}

class Vm;
class Node;

class VirtTools {
private:
	std::map<std::string, virConnectPtr> connections;
	boost::uuids::random_generator uuidGenerator;
	boost::random::random_device passwordDevice;
	std::string passwordChars;
	boost::random::uniform_int_distribution<> passwordDist;

public:
	VirtTools() {
		passwordChars =
				"abcdefghijklmnopqrstuvwxyz"
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"1234567890";
		passwordDist = boost::random::uniform_int_distribution<>(0, static_cast<int>(passwordChars.size()) - 1);
	};
	virtual ~VirtTools() {
		std::map<std::string, virConnectPtr>::iterator itConnections = connections.begin();
		std::map<std::string, virConnectPtr>::iterator lastIt;
		while (itConnections != connections.end()) {
			virConnectPtr conn = itConnections->second;
			virConnectClose(conn);
			lastIt = itConnections++;
			connections.erase(lastIt);
		}
	};

	virConnectPtr getConnection(const std::string nodeUri);

	bool checkVmsPerNode();

	bool checkVmsByNode(Node* node);

	void migrateVm(const Vm* vm, const Node* node, const std::string spicePort);

	const std::string generateUUID() {
//		char buffer[37];
//		sprintf(buffer, "%08lx-%04x-4%03x-%04x-%04x%04x%04x",
//				0xFFFFFFFF & time(NULL),
//				rand() % 0xFFFF,
//				rand() % 0x0FFF,
//				(rand() % 0xFFFF) & 0xBFFF,
//				rand() % 0xFFFF, rand() % 0xFFFF, rand() % 0xFFFF);
//		return std::string(buffer);
		boost::uuids::uuid u = uuidGenerator();
		return boost::uuids::to_string(u);
	};

	const std::string generateMacAddress() const {
		char buffer[18];
/*
		unsigned int random = 0xFFFFFF & time(NULL);
		sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x",
				0x52, 0x54, 0x00,
				(random & 0xFF0000) >> 16,
				(random & 0x00FF00) >> 8,
				(random & 0x0000FF));
*/
		struct timeval timeval;
		gettimeofday(&timeval, NULL);
		unsigned int secmask = 0xFFFFFF & timeval.tv_sec;
		unsigned int usecmask = 0xFFFFFF & timeval.tv_usec;
		sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x",
			0x52, 0x54, 0x00,
			(secmask & 0x0000FF),
			(usecmask & 0x00FF00) >> 8,
			(usecmask & 0x0000FF));
		return std::string(buffer);
	};

	const std::string generateSpicePassword() {
//		return generateUUID();
		std::string retval = "123456789012";
		for(size_t i = 0; i < retval.length(); ++i) {
			retval.at(i) = passwordChars[passwordDist(passwordDevice)];
		}
		return retval;
	};

	void createBackingStoreVolumeFile(const Vm* vm, const std::string& storagePoolName, const std::string& volumeName);
	void startVm(const Vm* vm);
	void stopVmForRestore(const Vm* vm);
	void destroyDynVm(const Vm* vm);

public:
	const std::string getVmXml(const Vm* vm) const;
	const std::string getBackingStoreVolumeXML(const Vm* vm, const std::string& volumeName) const;
};

class VirtException : public std::runtime_error {
public:
	VirtException(const std::string& _message) :
			std::runtime_error(_message) {
	}

	/**
	 * This method can be used to dump the data of a VirtException-Object.
	 * It is only useful for debugging purposes at the moment
	 */
	friend std::ostream& operator <<(std::ostream &s, const VirtException e);
};

#endif /* VIRTTOOLS_H_ */
