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
 * main.cpp
 *
 *  Created on: 22.03.2012
 *      Author: cwi
 */

#include <iostream>
#include "LDAPException.h"

#include "boost/program_options.hpp"

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
}

#include "include/signalHandler.hpp"
#include "include/logger.hpp"
#include "include/config.hpp"
#include "include/ldapTools.hpp"
#include "include/virtTools.hpp"
#include "include/vmPool.hpp"
#include "include/vmFactory.hpp"
#include "include/vm.hpp"
#include "include/node.hpp"
#include "include/evenlyPolicyInterval.hpp"

namespace po = boost::program_options;
using namespace std;

time_t getTime();
void logNodes(Config* config);
void printResults(Config* config);
void printNodeResults(Config* config);
void addMessageWeb();

#define DAEMON_NAME "fc-brokerd"
#define DAEMON_CONFIG_FILE "/etc/foss-cloud/broker.conf"
#define DAEMON_LOCK_FILE "/tmp/fc-brokerd.lock"
#define PWFCCK_FILE "/usr/sbin/pwfcck"
#define PWFCCK_LOG_FILE "/var/www/localhost/htdocs/vm-manager/protected/messages/.fc-message"
#define PWFCCK_CHECK_LOOP 20
#define DAEMON_VERSION "1.3.0.1"

int main(int argc, char* argv[]) {
    openlog(DAEMON_NAME, LOG_PID, LOG_DAEMON);

	string configfile = DAEMON_CONFIG_FILE;
	string configpool = "";
	try {
		po::options_description desc("Allowed options");
		string configDesc = "config file; default: ";
		configDesc.append(DAEMON_CONFIG_FILE);
		desc.add_options()
			("help", "produce this help message")
			("config,f", po::value<string>(), configDesc.c_str())
			("log,l", po::value<int>(), "log level (ERROR = 3, WARNING = 4, INFO = 6, DEBUG = 7); default: 3")
			("pool,p", po::value<string>(), "only use this pool (UUID); default: use all")
			("summary,s", po::value<string>(), "file to write summary for each loop; default: no file");

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			cout << DAEMON_NAME << "Version " << DAEMON_VERSION << endl;
			cout << desc << "\n";
			exit(EXIT_FAILURE);
		}

		if (vm.count("config")) {
			configfile = vm["config"].as<string>();
		}

		if (vm.count("log")) {
			switch (vm["log"].as<int>()) {
				case logERROR:
				SyslogLogger::setReportLevel(logERROR);
				break;
				case logWARNING:
				SyslogLogger::setReportLevel(logWARNING);
				break;
				case logINFO:
				SyslogLogger::setReportLevel(logINFO);
				break;
				case logDEBUG:
				SyslogLogger::setReportLevel(logDEBUG);
				break;
			}
		}
		else {
			SyslogLogger::setReportLevel(logERROR);
		}
		if (vm.count("pool")) {
			configpool = vm["pool"].as<string>();
		}
		if (vm.count("summary")) {
			FileLogger::setReportLevel(logDEBUG);
			LoggerOutputFile::FilePath() = vm["summary"].as<string>();
		}
	}
	catch (exception& e) {
		cerr << "error: " << e.what() << "\n";
		exit(EXIT_FAILURE);
	}
	catch (...) {
		cerr << "Exception of unknown type!\n";
	}

	SYSLOGLOGGER(logINFO) << "startup";


	/* Our process ID and Session ID */
	pid_t pid, sid;

	/* Fork off the parent process */

	pid = fork();
	if (pid < 0) {
		SYSLOGLOGGER(logINFO) << "Unable to fork";
		exit(EXIT_FAILURE);
	}

	/* If we got a good PID, then
	 we can exit the parent process. */

	if (pid > 0) {
		SYSLOGLOGGER(logINFO) << "Parent process ends now!";
		exit(EXIT_SUCCESS);
	}

	openlog(DAEMON_NAME, LOG_PID, LOG_DAEMON);

	SYSLOGLOGGER(logINFO) << "Child process running!";

	/* Change the file mode mask */
	umask(0);

	/* Create a new SID for the child process */

	sid = setsid();
	if (sid < 0) {
		// Log the failure
		SYSLOGLOGGER(logINFO) << "Unable to create new sid!";
		exit(EXIT_FAILURE);
	}

	/* Change the current working directory */
//	if ((chdir("/")) < 0) {
//		/* Log the failure */
//		exit(EXIT_FAILURE);
//	}

	/* Running a Single Copy */
	int lfp = open(DAEMON_LOCK_FILE, O_RDWR | O_CREAT, 0640);
	if (lfp < 0) {
		/* can not open */
		SYSLOGLOGGER(logINFO) << "already running!";
		exit(EXIT_FAILURE);
	}
	if (lockf(lfp, F_TLOCK, 0) < 0) {
		/* can not lock */
		exit(EXIT_FAILURE);
	}
	/* only first instance continues */

	char str[10];
	sprintf(str, "%d\n", getpid());
	/* record pid to lockfile */
	write(lfp, str, strlen(str));

	/* init SignalHandler */
	SignalHandler signalHandler;
	try {
		signalHandler.setupSignalHandlers();
	}
	catch (SignalException& e) {
		cerr << "SignalException: " << e.what() << endl;
		SYSLOGLOGGER(logINFO) << "Unable to initialize SignalHandler!";
		exit(EXIT_FAILURE);
	}

	SYSLOGLOGGER(logINFO) << "SignalHandler initialized!";

	Config::filename = configfile;
	Config* config = Config::getInstance();

	LdapTools* lt = new LdapTools();
	try {
		lt->bind();
		lt->readConfigurationSettings();
	}
	catch (LDAPException& e) {
		cout << "-------------- caught LDAPException ---------" << endl;
		cout << e << endl;
		SYSLOGLOGGER(logINFO) << "-------------- caught LDAPException ---------";
		SYSLOGLOGGER(logINFO) << e;
		return 1;
	}
	catch (std::exception& e) {
		cout << "-------------- caught exception ---------" << endl;
		cout << e.what() << endl;
		SYSLOGLOGGER(logINFO) << "-------------- caught exception ---------";
		SYSLOGLOGGER(logINFO) << e.what();
		return 1;
	}

	VirtTools* vt = new VirtTools();

	/* Close out the standard file descriptors */
	close(STDIN_FILENO);
//	close(STDOUT_FILENO);
	freopen( "/dev/null", "w", stdout);
//	close(STDERR_FILENO);
	freopen( "/dev/null", "w", stderr);

	map<string, VmPool*>* vmPools = config->getVmPools();
	map<string, VmPool*>::const_iterator it_pools;
	map<string, Vm*>* backupVms = config->getBackupVms();
	map<string, Vm*>::const_iterator it_backupVms;
	map<string, VmPool*>* shutdownVmPools = config->getShutdownVmPools();
	map<string, VmPool*>::const_iterator it_shutdownVmPools;
	VmPool* vmPool;
	Vm* vm;
	VmFactory vmFactory(lt, vt);
	bool doPolicy, doShutdown, doBackup;
	time_t actTime = getTime();
	int checkPWFCCK = 0;

	try {
		while (!signalHandler.gotExitSignal()) {
			doPolicy = doShutdown = doBackup = false;
			try {
				FILELOGGER(logINFO) << "-------------- next round! ---------";
				FILELOGGER(logINFO) << "------------------------------------";
				SYSLOGLOGGER(logINFO) << "-------------- next round! ---------";
				SYSLOGLOGGER(logINFO) << "------------------------------------";
				lt->readGlobalBackupConfiguration();
				lt->readVmPools(configpool, actTime);
				doPolicy = vt->checkVmsPerNode();
				doShutdown = 0 < shutdownVmPools->size();
				doBackup = 0 < backupVms->size();
				printResults(config);
				logNodes(config);
			}
			catch (LDAPException &e) {
				SYSLOGLOGGER(logERROR) << "Init -------------- caught LDAPException ---------";
				SYSLOGLOGGER(logERROR) << e;
				if (-1 == e.getResultCode()) {
					lt->unbind();
					SYSLOGLOGGER(logERROR) << "   try to bind again!";
					lt->bind();
				}
			}
			catch (VirtException &e) {
				SYSLOGLOGGER(logERROR) << "Init -------------- caught VirtException ---------";
				SYSLOGLOGGER(logERROR) << e;
			}
			catch(...) {
				SYSLOGLOGGER(logERROR) << "Init -------------- caught unknown ---------";
			}
			try {
				if (doPolicy) {
					for (it_pools = vmPools->begin(); it_pools != vmPools->end(); it_pools++) {
						vmPool = it_pools->second;

						FILELOGGER(logINFO) << "------------ check policy " << vmPool->getName();
						SYSLOGLOGGER(logINFO) << "------------ check policy " << vmPool->getName();

						BasePolicy* bp = const_cast<BasePolicy*>(vmPool->getPolicy());
						bp->checkPolicy(vmPool, &vmFactory, vt);
					}

					printNodeResults(config);
					logNodes(config);
				}
				else {
					SYSLOGLOGGER(logINFO) << "------------ no policy used -------";
				}
			}
			catch (LDAPException &e) {
				SYSLOGLOGGER(logERROR) << "Policy -------------- caught LDAPException ---------";
				SYSLOGLOGGER(logERROR) << e;
			}
			catch (VirtException &e) {
				SYSLOGLOGGER(logERROR) << "Policy -------------- caught VirtException ---------";
				SYSLOGLOGGER(logERROR) << e;
			}
			catch(...) {
				SYSLOGLOGGER(logERROR) << "Policy -------------- caught unknown ---------";
			}
			try {
				if (doShutdown) {
					for (it_shutdownVmPools = shutdownVmPools->begin(); it_shutdownVmPools != shutdownVmPools->end(); it_shutdownVmPools++) {
						vmPool = it_shutdownVmPools->second;

						FILELOGGER(logINFO) << "------------ handle shutdown " << vmPool->getName();
						SYSLOGLOGGER(logINFO) << "------------ handle shutdown " << vmPool->getName();

						vmPool->handleShutdown(vt);
					}
				}
				else {
					SYSLOGLOGGER(logINFO) << "Shutdown ------------ no shutdown handled -------";
				}
			}
			catch (LDAPException &e) {
				SYSLOGLOGGER(logERROR) << "Shutdown -------------- caught LDAPException ---------";
				SYSLOGLOGGER(logERROR) << e;
			}
			catch (std::exception &e) {
				SYSLOGLOGGER(logERROR) << "Shutdown -------------- caught std::exception ---------";
				SYSLOGLOGGER(logERROR) << e.what();
			}
			catch(...) {
				SYSLOGLOGGER(logERROR) << "Shutdown -------------- caught unknown ---------";
			}

			try {
				if (doBackup) {
					for (it_backupVms = backupVms->begin(); it_backupVms != backupVms->end(); it_backupVms++) {
						vm = it_backupVms->second;

						FILELOGGER(logINFO) << "------------ handle workflow " << vm->getName();
						SYSLOGLOGGER(logINFO) << "------------ handle workflow " << vm->getName();

						vm->handleBackupWorkflow(vt);
					}
				}
				else {
					SYSLOGLOGGER(logINFO) << "Backup ------------ no backup handled -------";
				}
			}
			catch (LDAPException &e) {
				SYSLOGLOGGER(logERROR) << "Backup -------------- caught LDAPException ---------";
				SYSLOGLOGGER(logERROR) << e;
			}
			catch (std::exception &e) {
				SYSLOGLOGGER(logERROR) << "Backup -------------- caught std::exception ---------";
				SYSLOGLOGGER(logERROR) << e.what();
			}
			catch(...) {
				SYSLOGLOGGER(logERROR) << "Backup -------------- caught unknown ---------";
			}

			if (0 == checkPWFCCK) {
				try {
					struct stat buffer;
					if (0 == stat(PWFCCK_FILE, &buffer)) {
						int retval = system(PWFCCK_FILE);
						SYSLOGLOGGER(logINFO) << "PWFCCK -------------- retval " << retval << " ---------";
					}
					else {
						addMessageWeb();
					}
				}
				catch (std::exception &e) {
					SYSLOGLOGGER(logERROR) << "PWFCCK -------------- caught std::exception ---------";
					SYSLOGLOGGER(logERROR) << e.what();
				}
				catch(...) {
					SYSLOGLOGGER(logERROR) << "PWFCCK -------------- caught unknown ---------";
				}
				checkPWFCCK = PWFCCK_CHECK_LOOP;
			}
			checkPWFCCK--;

			sleep(config->getCycle());
			actTime = getTime();

			FILELOGGER(logINFO) << "cleanup";
			SYSLOGLOGGER(logINFO) << "cleanup";
			config->clearMaps();
		}
	}
	catch(...) {
		SYSLOGLOGGER(logINFO) << "-------------- caught unknown after while ---------";
	}
	FILELOGGER(logINFO) << "terminated";
	SYSLOGLOGGER(logINFO) << "terminated";

	lt->unbind();
	delete lt;
	delete vt;

	lockf(lfp, F_ULOCK, 0);
	close(lfp);
	remove(DAEMON_LOCK_FILE);

	exit(EXIT_SUCCESS);
}

time_t getTime() {
	time_t rawtime;
	struct tm * timelocal;
	time(&rawtime);
	timelocal = localtime(&rawtime);
	timelocal->tm_sec = 0;
	return mktime(timelocal);
}

void logNodes(Config* config) {
	map<string, Node*>* cNodes = config->getNodes();
	map<string, Node*>::const_iterator itNodes = cNodes->begin();
	SYSLOGLOGGER(logINFO) << "-------------------- Nodes:";
	for (; itNodes != cNodes->end(); itNodes++) {
		const Node* node = itNodes->second;
		node->logging();
	}
}

void addMessageWeb() {
	std::ofstream out(PWFCCK_LOG_FILE, std::ios_base::trunc);
	out << "<p style=\"margin-bottom:5px;margin-top:7px;text-align:center;letter-spacing:0.4em;font-weight:bold;color:red\">WARNING!</p>Unable to call " << PWFCCK_FILE <<
			". File not found! Please contact <a href=\"mailto:support@foss-group.com\">support@foss-group.com</a>.";
	out.close();
}

void printResults(Config* config) {
	if (0 == LoggerOutputFile::FilePath().length()) {
		return;
	}
	FILELOGGER(logINFO) << "=========== RESULTS ===========";
	map<string, VmPool*>* cVmPools = config->getVmPools();
	map<string, VmPool*>::const_iterator itVmPools = cVmPools->begin();
	FILELOGGER(logINFO) << "-------------------- VmPools:";
	for (; itVmPools != cVmPools->end(); itVmPools++) {
		FILELOGGER(logINFO) << itVmPools->first << ": " << *(itVmPools->second);
	}

	map<string, Vm*>* cVms = config->getVms();
	map<string, Vm*>::const_iterator itVms = cVms->begin();
	FILELOGGER(logINFO) << "-------------------- Vms:";
	for (; itVms != cVms->end(); itVms++) {
		FILELOGGER(logINFO) << *(itVms->second);
	}

	printNodeResults(config);

	map<string, Vm*>* cBackupVms = config->getBackupVms();
	map<string, Vm*>::const_iterator itBackupVms = cBackupVms->begin();
	FILELOGGER(logINFO) << "-------------------- Backup Vms:";
	for (; itBackupVms != cBackupVms->end(); itBackupVms++) {
		FILELOGGER(logINFO) << *(itBackupVms->second);
	}
}

void printNodeResults(Config* config) {
	if (0 == LoggerOutputFile::FilePath().length()) {
		return;
	}
	map<string, Node*>* cNodes = config->getNodes();
	map<string, Node*>::const_iterator itNodes = cNodes->begin();
	FILELOGGER(logINFO) << "-------------------- Nodes:";
	for (; itNodes != cNodes->end(); itNodes++) {
		FILELOGGER(logINFO) << itNodes->first << ": " << *(itNodes->second);
	}
}
