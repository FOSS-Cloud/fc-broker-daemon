/*
 * Copyright (C) 2013 FOSS-Group
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
 * basePolicy.cpp
 *
 *  Created on: 11.11.2013
 *      Author: cwi
 */

#include <iostream>
#include <cstdio>
#include <algorithm>

#include "include/evenlyPolicy.hpp"
#include "include/evenlyPolicyInterval.hpp"

using namespace std;

ostream& operator <<(ostream& s, const BasePolicy& policy) {
	const EvenlyPolicyInterval *epi = reinterpret_cast<const EvenlyPolicyInterval*>(&policy);
	if (NULL == epi) {
		const EvenlyPolicy *ep = reinterpret_cast<const EvenlyPolicy*>(&policy);
		if (NULL != ep) {
			s << *ep;
		}
		else {
			s << "unknown policy!";
		}
	}
	else {
		s << *epi;
	}
	return s;
}

