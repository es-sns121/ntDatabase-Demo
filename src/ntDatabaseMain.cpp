/*	
 *	============================================================
 *	
 *	Author: Evan Smith
 *	Date  : 6/5/2017
 * 	
 * 	ntDatabaseMain.cpp
 *
 * 	Main source file of normative type database demo 
 * 	program. The programs purpose is to instantiate
 * 	and host a record containing each normative type
 * 	pv structure on an EPICS v4 database. There will be 
 *  one pvRecord for each normative type. The records'
 *  functionalities will then be demonstrated as they will 
 *  be written to and read from over the network from a 
 *  client program (ntDatabaseClient.cpp).
 *
 *	============================================================
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <pv/channelProviderLocal.h>
#include <pv/serverContext.h>

#include <pv/ntDatabase.h>

using namespace std;

using std::tr1::static_pointer_cast;
using namespace epics::pvData;
using namespace epics::pvAccess;
using namespace epics::pvDatabase;
using namespace epics::ntDatabase;

int main (int argc, char **argv)
{

	bool verbosity(false);

	if (argc > 1 && argv[1]) {
		
		string arg(argv[1]);	
		
		if (arg == string("-v")) {
		/* Verbose flag */
			verbosity = true;
		
		} else if (arg == string("-h")) {
		/* Help flag */	
			cout << "Help -- executable flags -- only one accepted at a time" << endl
				 << "\t -v (verbose. prints database record names.)\n"
				 << "\t -h (help. prints help information)\n";
		
			return 0;
		} else {
		/* Error */
			cout << "unrecognized flag: \"" << arg << "\" (use -h for help)." << endl;
			return 0;
		
		}
	}
	
	// Get the master database maintained by the local channel provider.
	PVDatabasePtr master = PVDatabase::getMaster();
	
	// Get the local channel provider.
	ChannelProviderLocalPtr cpLocal = getChannelProviderLocal();

	// Create the normative type database that is defined locally in pv/ntDatabase.h
	NTDatabase::create();

	// After the records are added to the database, start the server. 
	ServerContext::shared_pointer pvaServer =
		startPVAServer("local", 0, true, true);
	
	// print the record names currently hosted in the database
	if (verbosity) {
		shared_vector<const string> record_names = master->getRecordNames()->view();
		
		cout << "Records currently hosted:\n";
		for (size_t i = 0; i < record_names.size(); i++) {
			cout << "\t" << record_names[i] << endl;
		}
	}

	// Clear the pointer.
	master.reset();

	// Wait to die.
	string input;
	while (true)
	{
		cout << "Type exit to stop: \n";
		getline(cin, input);
		if (input.compare("exit") == 0) 
		{
			break;
		}
	}

	// Clean up so that we can exit cleanly.
	pvaServer->shutdown();
	pvaServer->destroy();
	cpLocal->destroy();

	return 0;
}

