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
 * signalHandler.hpp
 *
 *  Created on: 22.03.2012
 *      Author: cwi
 */

#ifndef SIGNALHANDLER_HPP_
#define SIGNALHANDLER_HPP_

#include <stdexcept>

class SignalHandler {
protected:
	static bool exitSignal;

public:
	SignalHandler() {};
	~SignalHandler() {};

	/**
	 * Returns the bool flag indicating whether we received an exit signal
	 * @return Flag indicating shutdown of program
	 */
	static bool gotExitSignal() {
		return exitSignal;
	};

	/**
	 * Sets the bool flag indicating whether we received an exit signal
	 */
	static void setExitSignal(bool signal) {
		exitSignal = signal;
	};

	void setupSignalHandlers();

	/**
	 * Sets exit signal to true.
	 * @param[in] _ignored Not used but required by function prototype
	 *                     to match required handler.
	 */
	static void exitSignalHandler(int ignored) {
		exitSignal = true;
	}

};

class SignalException : public std::runtime_error {
public:
	SignalException(const std::string& _message) :
			std::runtime_error(_message) {
	}
};

#endif /* SIGNALHANDLER_HPP_ */
