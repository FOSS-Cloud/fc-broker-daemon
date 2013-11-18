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
 * NodeGen.h
 *
 *  Created on: Jun 8, 2012
 *      Author: wre
 */

#ifndef FACTORY_HPP_
#define FACTORY_HPP_

#include <list>
#include <string>
#include <sstream>
#include <iostream>
#include <stdio.h>

template<typename T> class Factory
{
	std::list<T*> instances;
	int index;
	std::string prefix;
public:
	Factory(std::string prefix_)
	: index(0),
	  prefix(prefix_)
	{
	}
	virtual ~Factory() {}

	T* createInstance()
	{
		std::stringstream sb;
		sb.width(3);
		sb.fill('0');
		sb << index++;
//		std::cout << "factory:" << prefix << sb.str() << std::endl;
		return new T(prefix, prefix + sb.str());
	}

	void reset()
	{
		for (typename std::list<T*>::iterator it = instances.begin(); it != instances.end();)
		{
			delete *it;
		}
		instances.clear();
		index = 0;
	}
};

#endif /* FACTORY_HPP_ */
