#include "module.h"
#include "hashstore.h"

CModule::CModule (CInputParser& parser) {

	while (true) {
		
		CToken token = parser.getToken ();
		if (token.getTokenType () == EndOfFile)
			break;
		
		if (token.getValue () == "function" || token.getValue () == "target") {
		
			CToken nameToken = parser.getToken ();
			if (nameToken.getTokenType () != NameToken) 
				throw ESyntaxError (parser, "Expected function name");
			
			string functionName = nameToken.getValue ();
			
			CToken next = parser.getToken ();
			if (next.getValue () != "(")
				throw ESyntaxError (parser, "Expected (");
		
			list<string> args;
			
			while (true) {
				
				next = parser.getToken ();
				if (next.getValue () == ")")				
					break;
				
				if (next.getTokenType() != NameToken)
					throw ESyntaxError (parser, "Expected parameter name");
				
				args.push_back (next.getValue ());
				
				next = parser.getToken ();
				if (next.getValue () == ")")
					break;
				else if (next.getValue () != ",")
					throw ESyntaxError (parser, "Expected , or )");
			}
			
			shared_ptr<CStatement> stmt = CStatement::parse (parser);
			shared_ptr<CUserFunction> userFunc (new CUserFunction (functionName, args, stmt, (token.getValue() == "target")));

			addUserFunction (functionName, userFunc);
			if (token.getValue () == "target")
				targets.push_back (functionName);
			
		} else {
		
			parser.pushBack (token);
			shared_ptr<CStatement> stmt = CStatement::parse (parser);
			stmts.push_back (stmt);
			
		}
		
	}

	
}

void CModule::addFunctionsToContext (shared_ptr<CExecutionContext> ctx, bool includeTargets) {
	
	if (includeTargets)
		ctx -> getBaseContext() -> addFunctions (userFunctions);
	else {
		map<string,shared_ptr<CUserFunction>> onlyFuncs;
		for (map<string,shared_ptr<CUserFunction>>::iterator it = userFunctions.begin (); it != userFunctions.end (); it++)
			onlyFuncs.insert (*it);
		
		for (list<string>::iterator it = targets.begin (); it != targets.end (); it++)
			onlyFuncs.erase (*it);
		
		ctx -> getBaseContext() -> addFunctions (onlyFuncs);
	}
	
}

void CModule::execute (shared_ptr<CExecutionContext> ctx) {

	for (list<shared_ptr<CStatement>>::iterator it = stmts.begin(); it != stmts.end (); it ++) {
		(*it) -> execute (ctx);
	}
	
}

bool CModule::hasTarget (const string& targetName) {

	for (list<string>::iterator it = targets.begin (); it != targets.end (); it++) {
		if ((*it) == targetName)
			return true;
	}
	
	return false;
	
}

shared_ptr<CValue> CModule::executeTarget (shared_ptr<CExecutionContext> ctx, const string& targetName, const list<string>& params) {

	map<string,shared_ptr<CUserFunction>>::iterator f_it = userFunctions.find (targetName);
	if (f_it == userFunctions.end ())
		throw runtime_error ("No such function");
	
	shared_ptr<CUserFunction> func = f_it -> second;
	
	vector<shared_ptr<CExpression>> invoke_args;
	for (list<string>::const_iterator it = params.begin (); it != params.end (); it++) {
		shared_ptr<CExpression> arg = shared_ptr<CExpression> (new CConstantExpression (shared_ptr<CValue> (new CStringValue (*it))));
		invoke_args.push_back (arg);
	}
	
	shared_ptr<CValue> retValue = func -> execute (ctx, invoke_args);
	
	if (targetName == "clean") 
		hashStore.clear (ctx -> getCurModule ());
	
	return retValue;
	
}

void CModule::listTargets () {
	for (list<string>::iterator it = targets.begin (); it != targets.end (); it++)
		cout << (*it) << endl;
}







