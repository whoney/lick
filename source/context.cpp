#include "context.h"
#include "value.h"
#include "sys_funcs.h"

void CVarStore::initDefaultVars () {

	CDictValue *sys = new CDictValue ();
	
	sys -> append ("platform", shared_ptr<CValueRef> (new CValueRef (shared_ptr<CValue> (new CStringValue (getPlatform ())))));
	sys -> append ("hostname", shared_ptr<CValueRef> (new CValueRef (shared_ptr<CValue> (new CStringValue (getComputerName ())))));
	sys -> append ("username", shared_ptr<CValueRef> (new CValueRef (shared_ptr<CValue> (new CStringValue (getUserName ())))));
	sys -> append ("bits", shared_ptr<CValueRef> (new CValueRef (shared_ptr<CValue> (new CIntValue (getBitness ())))));
	
	CDictValue *env = new CDictValue ();
	CArrayValue *path = new CArrayValue ();
	
	map<string,string> envmap;
	getEnvironment (envmap);
	
	for (map<string,string>::iterator it = envmap.begin (); it != envmap.end (); it++) {
	
		string key = it -> first;
		string value = it -> second;
		
		if (equalsIgnoreCase (key, "path")) {
			size_t pos = 0;
			size_t nextSep;
			
			while (pos < value.length()) {
				nextSep = value.find (ENV_PATH_SEPARATOR, pos);
				if (nextSep == string::npos)
					nextSep = value.length ();
					
				string pathComp = value.substr (pos, nextSep - pos);
				path -> append (shared_ptr<CValueRef> (new CValueRef (shared_ptr<CValue> (new CStringValue (pathComp)))));
				pos = nextSep + 1;
			}
		
		} else {
			env -> append (key, shared_ptr<CValueRef> (new CValueRef (shared_ptr<CValue> (new CStringValue (value)))));
		}
	
	}

	sys -> append ("env", shared_ptr<CValueRef> (new CValueRef (shared_ptr<CValue> (env))));
	sys -> append ("path", shared_ptr<CValueRef> (new CValueRef (shared_ptr<CValue> (path))));
	
	setVar ("sys", shared_ptr<CValue> (sys));

}

shared_ptr<CValue> CVarStore::getVar (const string& varName) {

	map<string, shared_ptr<CValueRef>>::iterator it = variables.find (varName);
	if (it == variables.end ()) {
		if (baseStore)
			return baseStore -> getVar (varName);
		else
			return shared_ptr<CValue> (new CVoidValue ());
	}
	
	return it -> second -> getValue ();
}


shared_ptr<CValueRef> CVarStore::getVarRef (const string& varName) {

	map<string, shared_ptr<CValueRef>>::iterator it = variables.find (varName);
	
	if (it == variables.end ()) {
		
		shared_ptr<CValue> newValue = getVar (varName);
		shared_ptr<CValueRef> newRef = shared_ptr<CValueRef> (new CValueRef (newValue));
		variables.insert (pair<string, shared_ptr<CValueRef>> (varName, newRef));
		
		return newRef;
	}
	
	return it -> second;

}

void CVarStore::setVar (const string& varName, shared_ptr<CValue> value) {

	map<string, shared_ptr<CValueRef>>::iterator it = variables.find (varName);
	
	if (it == variables.end ()) {
		
		shared_ptr<CValue> newValue = value;
		shared_ptr<CValueRef> newRef = shared_ptr<CValueRef> (new CValueRef (newValue));
		variables.insert (pair<string, shared_ptr<CValueRef>> (varName, newRef));
		
	} else 

		it -> second -> setValue (value);
	
}


void CBaseExecutionContext::addFunctions (const map<string,shared_ptr<CUserFunction>>& moreFunctions) {

	for (map<string,shared_ptr<CUserFunction>>::const_iterator it = moreFunctions.begin (); it != moreFunctions.end (); it++) {
		map<string,shared_ptr<CUserFunction>>::iterator old = userFunctions.find (it -> first);
		if (old != userFunctions.end ())
			throw runtime_error ("Function name conflict");
		userFunctions.insert (*it);
	}
	
}

shared_ptr<CUserFunction> CBaseExecutionContext::getFunction (const string& name) {

	map<string,shared_ptr<CUserFunction>>::iterator it = userFunctions.find (name);
	if (it == userFunctions.end ())
		throw runtime_error ("Function does not exist");
	
	return it -> second;
	
}

bool CBaseExecutionContext::hasIncludedModule (const string& path) {
	
	set<string>::iterator it = includedModules.find (path);
	return (it != includedModules.end ());
	
}

void CBaseExecutionContext::addIncludedModule (const string& path) {
	
	includedModules.insert (path);
	
}

shared_ptr<CValue> CUserFunction::execute (shared_ptr<CExecutionContext> ctx, const vector<shared_ptr<CExpression>>& invoke_args) {

	if (args.size() != invoke_args.size ())
		throw runtime_error ("Invalid number of arguments");
	
	shared_ptr<CExecutionContext> newCtx (ctx -> enterFunction (functionName, isTarget));
	
	list<string>::iterator it_names = args.begin ();
	vector<shared_ptr<CExpression>>::const_iterator it_exprs = invoke_args.begin ();
	
	while (it_names != args.end () && it_exprs != invoke_args.end ()) {
		
		shared_ptr<CValue> argValue = (*it_exprs) -> evaluate (ctx);
		newCtx -> getVarStore () -> setVar (*it_names, argValue);
		
		it_names ++;
		it_exprs ++;
		
	}
	
	stmt -> execute (newCtx);
	
	return newCtx -> getReturnValue ();
	
}

void CUserFunction::updateHash (shared_ptr<CExecutionContext> ctx, SHA1& hash) {

	for (list<string>::iterator it = args.begin (); it != args.end (); it ++) {
		hash.update (":arg:");
		hash.update (*it);
	}
	
	hash.update (":body:");
	stmt -> updateHash (ctx, hash);
	
}




