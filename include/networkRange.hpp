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
 * networkRange.hpp
 *
 *  Created on: 25.06.2012
 *      Author: cwi
 */

#ifndef NETWORKRANGE_HPP_
#define NETWORKRANGE_HPP_

#include <string>
#include <sstream>
#include <vector>
#include <cassert>
#include <iostream>
#include <sstream>

#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"

class NetworkRange {
private:
	std::string origDn;
	std::string dn;
	std::string range;
	int netmask;
	std::string network;
	std::string broadcast;
	std::string hostmin;
	std::string hostmax;

public:
	NetworkRange(const std::string& origDn_, const std::string& dn_, const std::string& range_) {
		origDn = origDn_;
		dn = dn_;
		range = range_;
		std::vector < std::string > cdir;
		boost::algorithm::split(cdir, range, boost::algorithm::is_any_of("/"), boost::algorithm::token_compress_off);
		assert(2 == cdir.size());
		if (2 == cdir.size()) {
			netmask = atoi(cdir[1].c_str());
			std::string address = cdir[0];
			std::vector < std::string > parts;
			boost::algorithm::split(parts, address, boost::is_any_of("."), boost::algorithm::token_compress_off);
			while (4 > parts.size()) {
				parts.push_back("0");
			}
			char buffer[16];
			sprintf(buffer, "%u.%u.%u.%u", atoi(parts[0].c_str()), atoi(parts[1].c_str()), atoi(parts[2].c_str()),
					atoi(parts[3].c_str()));
			address = std::string(buffer);
			long ip_dec = ip2long(address);

			long wildcard_dec = (1 << (32 - netmask)) - 1;
			long netmask_dec = ~wildcard_dec;

			long network_dec = ip_dec & netmask_dec;
			long broadcast_dec = ip_dec | wildcard_dec;

			long hostmin_dec = network_dec + 1;
			long hostmax_dec = broadcast_dec - 1;

			network = long2ip(network_dec);
			broadcast = long2ip(broadcast_dec);
			hostmin = long2ip(hostmin_dec);
			hostmax = long2ip(hostmax_dec);
		}
	}

	virtual ~NetworkRange() {};

	bool addAttribute(const std::string& actDn, const std::string& attr, const std::string& val) {
		return true;
	}
	;

	const std::string& getOrigDn() const {
		return origDn;
	}
	const std::string& getDn() const {
		return dn;
	}
	const std::string& getRange() const {
		return range;
	}
	const std::string& getHostMin() const {
		return hostmin;
	}
	const std::string& getHostMax() const {
		return hostmax;
	}

	static long ip2long(const std::string& address) {
		long retval = 0;
		std::vector < std::string > parts;
		boost::algorithm::split(parts, address, boost::algorithm::is_any_of("."), boost::algorithm::token_compress_off);
		assert(4 == parts.size());
		if (4 == parts.size()) {
			for (std::vector<std::string>::const_iterator it = parts.begin(); it != parts.end(); it++) {
				retval *= 256;
				retval += atoi((*it).c_str());
			}
		}
		return retval;
	}

	static const std::string long2ip(long val) {
		std::stringstream sstream;
		long c = 0x1000000;
		for (int i = 0; i < 4; i++) {
			long k = val / c;
			val -= c * k;
			if (0 < i) {
				sstream << ".";
			}
			sstream << k;
			c /= 0x100;
		};
		return sstream.str();
	}

	friend std::ostream& operator <<(std::ostream& s, const NetworkRange& range);
};

#endif /* NETWORKRANGE_HPP_ */
