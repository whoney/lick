#ifndef __MODULE_H__
#define __MODULE_H__

#include <list>
#include <string>
#include <map>
#include <memory>
#include <stdexcept>

#include "parser.h"
#include "stmt.h"
#include "context.h"

using namespace std;

class CModule {
	
	private:
		
		map<string,shared_ptr<CUserFunction>> userFunctions;
		list<shared_ptr<CStatement>> stmts;
		list<string> targets;
		
		void addUserFunction (const string& name, shared_ptr<CUserFunction> func) {
			
			map<string,shared_ptr<CUserFunction>>::iterator it = userFunctions.find (name);
			if (it != userFunctions.end ())
				throw runtime_error ("Function already exists");
			
			userFunctions.insert (pair<string,shared_ptr<CUserFunction>> (name, func));
		}
		
	public:
		
		CModule (CInputParser& parser);
		
		void addFunctionsToContext (shared_ptr<CExecutionContext> ctx, bool includeTargets);
		
		shared_ptr<CUserFunction> getUserFunction (const string& name) {

			map<string,shared_ptr<CUserFunction>>::iterator it = userFunctions.find (name);
			if (it == userFunctions.end ())
				throw runtime_error ("Function does not exist");
			
			return it -> second;
			
		}
		
		void execute (shared_ptr<CExecutionContext> ctx);
		bool hasTarget (const string& targetName);
		void listTargets ();
		shared_ptr<CValue> executeTarget (shared_ptr<CExecutionContext> ctx, const string& targetName, const list<string>& params);
	
};


#endif /* __MODULE_H__ */

