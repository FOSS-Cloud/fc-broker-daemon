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
 * ldapData.hpp
 *
 *  Created on: 14.05.2012
 *      Author: cwi
 */

#ifndef LDAPDATA_HPP_
#define LDAPDATA_HPP_

#include <string>
#include <vector>
#include <iostream>
extern "C" {
#include "ldap.h"
}

class LdapTools;

class LdapData {
protected:
	std::string dn;
	LdapTools* lt;
	std::string customerUID;
	std::string resellerUID;

public:
	LdapData(std::string dn_) : dn(dn_), lt(NULL) {};
	LdapData(std::string dn_, LdapTools* lt_) : dn(dn_), lt(lt_) {};
	virtual ~LdapData() {};

	const std::string& getDn() const {
		return dn;
	};
	const std::string& getCustomerUID() const {
		return customerUID;
	};
	const std::string& getResellerUID() const {
		return resellerUID;
	};

	void getDnPart(const std::string& part, std::string& retval) const {
		getDnPart(dn, part, retval);
	}

	void getDnPart(const std::string& dn_, const std::string& part, std::string& retval) const {
		std::string p(part);
		p.append("=");
		size_t pos = dn_.find(p);
		//std::cout << "pos: " << pos << endl;
		if (std::string::npos != pos) {
			size_t start = pos+p.length();
			//std::cout << "start: " << start << endl;
			size_t length = (dn_.find_first_of(",", start)) - start;
			//std::cout << "length: " << length << endl;
			retval = std::string(dn_, start, length);
		}
	}

	void getDnParent(std::string& retval, int steps=1) const {
		getDnParent(dn, retval, steps);
	}

	void getDnParent(const std::string& dn_, std::string& retval, int steps=1) const {
		size_t pos = -1;
		for (int i=0; i<steps; i++) {
			pos = dn_.find(",", pos+1);
		}
		retval = std::string(dn_, pos+1);
	}

	virtual bool addAttribute(const std::string& actDn, const std::string& attr, const std::string& val) = 0;

};

#endif /* LDAPDATA_HPP_ */
