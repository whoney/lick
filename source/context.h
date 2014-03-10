#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <string>
#include <map>
#include <memory>
#include <list>
#include <set>

#include "value.h"

using namespace std;

class CVarStore {
	
	private:
		
		shared_ptr<CVarStore> baseStore;
		map<string, shared_ptr<CValueRef>> variables;
		
	private:
	
		void initDefaultVars ();
		
	public:
		
		CVarStore () { 
			initDefaultVars ();
		};
		
		CVarStore (shared_ptr<CVarStore> p_baseStore): baseStore (p_baseStore) { }
		
		shared_ptr<CValue> getVar (const string& varName);
		shared_ptr<CValueRef> getVarRef (const string& varName);
		void setVar (const string& varName, shared_ptr<CValue> value);
	
};

class CStatement;
class CExecutionContext;
class CExpression;

class CUserFunction {
	
	private:	
	
		string functionName;
		bool isTarget;
		
		list<string> args;
		shared_ptr<CStatement> stmt;
	
	public:
		
		CUserFunction (const string& p_functionName, const list<string>& p_args, shared_ptr<CStatement> p_stmt, bool p_isTarget):
			functionName (p_functionName),
			isTarget (p_isTarget),
			args (p_args),
			stmt (p_stmt) { }
			
		shared_ptr<CValue> execute (shared_ptr<CExecutionContext> ctx, const vector<shared_ptr<CExpression>>& invoke_args);

		void updateHash (shared_ptr<CExecutionContext> ctx, SHA1& hash);
	
};

class CBaseExecutionContext {
	
	private:
		
		map<string,shared_ptr<CUserFunction>> userFunctions;
		set<string> includedModules;
		
	public:
		
		void addFunctions (const map<string,shared_ptr<CUserFunction>>& moreFunctions);
		shared_ptr<CUserFunction> getFunction (const string& name);
		
		bool hasIncludedModule (const string& path);
		void addIncludedModule (const string& path);
		
};


class CExecutionContext {
	
	private:
		
		shared_ptr<CBaseExecutionContext> baseContext;
		shared_ptr<CVarStore> varStore;
		shared_ptr<CValueRef> retValue;
		
		shared_ptr<string> curModule;
		string curTarget;
		
	public:
		
		bool breakSignaled;
		bool continueSignaled;
		bool returnSignaled;
		
		CExecutionContext (const string& p_moduleFullPath) {
			varStore = shared_ptr<CVarStore> (new CVarStore ());
			baseContext = shared_ptr<CBaseExecutionContext> (new CBaseExecutionContext ());
			retValue = shared_ptr<CValueRef> (new CValueRef (shared_ptr<CValue> (new CVoidValue ())));
			breakSignaled = false;
			continueSignaled = false;
			returnSignaled = false;
			
			curModule = shared_ptr<string> (new string (p_moduleFullPath));
			curTarget = "";
		}
		
		CExecutionContext (const CExecutionContext& ctx) {
			varStore = ctx.varStore;
			baseContext = ctx.baseContext;
			breakSignaled = ctx.breakSignaled;
			continueSignaled = ctx.continueSignaled;
			returnSignaled = ctx.returnSignaled;
			retValue = ctx.retValue;
			curModule = ctx.curModule;
			curTarget = ctx.curTarget;
		}
		
		string getCurModule () {
			return (*curModule);
		}
		
		string getCurTarget () {
			return curTarget;
		}
		
		shared_ptr<CExecutionContext> enterFunction (const string& functionName, bool isTarget) {
			
			CExecutionContext *newCtx = new CExecutionContext (*this);
			newCtx -> varStore = shared_ptr<CVarStore> (new CVarStore (varStore));
			newCtx -> retValue = shared_ptr<CValueRef> (new CValueRef (shared_ptr<CValue> (new CVoidValue ())));
			newCtx -> returnSignaled = false;
			
			if (isTarget)
				newCtx -> curTarget = functionName;
			
			return shared_ptr<CExecutionContext> (newCtx);
			
		}
		
		void returnValue (shared_ptr<CValue> p_retValue) {
			retValue -> setValue (p_retValue);
			returnSignaled = true;
		}
		
		shared_ptr<CValue> getReturnValue () {
			return retValue -> getValue ();
		}
		
		shared_ptr<CVarStore> getVarStore () {
			return varStore;
		}
		
		shared_ptr<CBaseExecutionContext> getBaseContext () {
			return baseContext;
		}
		
	
};

#include "stmt.h"


#endif /* __CONTEXT_H__ */
