#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <memory>

#include "parser.h"
#include "expr.h"
#include "stmt.h"
#include "module.h"
#include "hashstore.h"

using namespace std;

void usage () {

	cerr << "Use: lick [-f <input file>] [target [target-args...]]" << endl;
	
}

int main (int argc, char *argv[]) {
	
	string inputFile = "lickable";
	list<string> params;
	
	for (int i = 1; i < argc; i++) {
		string arg (argv[i]);
		
		if (arg == "-h" || arg == "--help" || arg == "/?") {
			usage ();
			return 1;
		}
		
		if (arg == "-f") {
			i++;
			if (i < argc) {
				inputFile = argv[i];
			} else {
				usage ();
				return 1;
			}
		} else
			params.push_back (arg);
		
	}
	
	try {

		CLineCountedInputFile input (inputFile);
		CInputParser parser (input);
	
		CModule module (parser);

		string willExecuteTarget;
		list<string> willExecuteParams;
		
		if (!params.empty ()) {
			string requestedTarget = params.front ();
			
			if (requestedTarget == "what") {
				module.listTargets ();
				return 1;
			}
			
			if (module.hasTarget (requestedTarget)) {
				willExecuteTarget = requestedTarget;
				list<string>::iterator it = params.begin ();
				it ++;
				while (it != params.end ()) {
					willExecuteParams.push_back (*it);
					it ++;
				}
			} else
				throw runtime_error ("Target " + requestedTarget + " is not defined.");
		} else {
			if (module.hasTarget ("default"))
				willExecuteTarget = "default";
			else if (module.hasTarget ("all"))
				willExecuteTarget = "all";
		}
		
		string moduleFullPath = getAbsolutePath (makeSysSeparators (inputFile));
		
		shared_ptr<CExecutionContext> ctx (new CExecutionContext (moduleFullPath));
		
		ctx -> getBaseContext () -> addIncludedModule (moduleFullPath);
		
		module.addFunctionsToContext (ctx, true);
		module.execute (ctx);
		
		if (!willExecuteTarget.empty ()) {
			module.executeTarget (ctx, willExecuteTarget, willExecuteParams);
		}
			
	} catch (exception& e) {
		cerr << e.what () << endl;
		return 1;
	}
	
	return 0;

}

