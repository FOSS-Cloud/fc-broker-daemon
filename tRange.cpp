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
 * tRange.cpp
 *
 *  Created on: Jun 24, 2012
 *      Author: cwi
 */

#include <iostream>
#include <cstdio>

#include "include/tRange.hpp"
#include "include/networkRange.hpp"

using namespace std;

void TRange::runTest()
{
	cout << "TRange::runTest()" << endl;

	NetworkRange range = NetworkRange("192.168.141.16/27", "aDn");
	cout << "Range: " << range << endl;
}
