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
 * networkRange.cpp
 *
 *  Created on: 25.06.2012
 *      Author: cwi
 */

#include "include/networkRange.hpp"

using namespace std;

ostream& operator <<(ostream& s, const NetworkRange& range) {
	s << range.range << ": " << range.netmask << endl;
	s << "     OrigDn:    " << range.origDn << endl;
	s << "     Dn:        " << range.dn << endl;
	s << "     Network:   " << range.network << endl;
	s << "     Broadcast: " << range.broadcast << endl;
	s << "     HostMin:   " << range.hostmin << endl;
	s << "     HostMax:   " << range.hostmax << endl;

	return s;
}
