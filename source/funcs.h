#ifndef __FUNCS_H__
#define __FUNCS_H__

#include <string>
#include <vector>
#include <memory>
#include <list>

#include "parser.h"
#include "expr.h"

class CFunctionCall: public CExpression {

	protected:
		
		vector<shared_ptr<CExpression>> args;
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		
	private:
		
		static shared_ptr<CFunctionCall> getBuiltinFunction (const string& name, const vector<shared_ptr<CExpression>>& args);
		
	public:
		
		CFunctionCall (const vector<shared_ptr<CExpression>>& p_args): args (p_args) { }
		
		static shared_ptr<CFunctionCall> makeFunctionCall (const string& name, const vector<shared_ptr<CExpression>>& args);
	
};

class CUserFunctionCall: public CFunctionCall {
	
	private:
		
		string userFuncName;

	protected:
	
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		
	public:
		
		CUserFunctionCall (const vector<shared_ptr<CExpression>>& p_args, const string& p_userFuncName): CFunctionCall (p_args) {
			userFuncName = p_userFuncName;
		}
		
		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
	
};

class CFuncStrlen: public CFunctionCall {
	
	public:
		
		CFuncStrlen (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 1)
				throw runtime_error ("strlen() expects 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx) {
			shared_ptr<CValue> arg = args[0] -> evaluate (ctx);
			string str = arg -> asString ();
			return shared_ptr<CValue> (new CIntValue (str.length()));
		}
		
};

class CFuncLength: public CFunctionCall {
	
	public:
		
		CFuncLength (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 1)
				throw runtime_error ("length() expects 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx) {
			shared_ptr<CValue> arg = args[0] -> evaluate (ctx);
			int len = arg -> getLength ();
			return shared_ptr<CValue> (new CIntValue (len));
		}
		
};

class CFuncPrint: public CFunctionCall {
	
	private:
		
		bool addNewLine;
	
	public:
		
		CFuncPrint (const vector<shared_ptr<CExpression>>& p_args, bool p_addNewLine): CFunctionCall (p_args) {
			addNewLine = p_addNewLine;
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx) {
			
			for (vector<shared_ptr<CExpression>>::iterator it = args.begin (); it != args.end (); it++) {
				shared_ptr<CValue> arg = (*it) -> evaluate (ctx);
				cout << arg -> asString ();
			}
			
			if (addNewLine)
				cout << endl;
			
			return shared_ptr<CValue> (new CVoidValue ());
		}
		
};

class CFuncFiles: public CFunctionCall {
	
	public:
		
		CFuncFiles (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 1)
				throw runtime_error ("files() expects 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncReadFile: public CFunctionCall {
	
	public:
		
		CFuncReadFile (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 1)
				throw runtime_error ("readfile() expects 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncWriteFile: public CFunctionCall {
	
	public:
		
		CFuncWriteFile (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 2)
				throw runtime_error ("writefile() expects 2 parameters");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};


class CFuncMatch: public CFunctionCall {
	
	private:
		
		bool isExclude;
		
		bool testMatch (const string& value, const string& pattern);
		bool matchOne (shared_ptr<CExecutionContext> ctx, shared_ptr<CValue> value);
	
	public:
		
		CFuncMatch (const vector<shared_ptr<CExpression>>& p_args, bool p_isExclude): CFunctionCall (p_args) {
			if (args.size () < 2)
				throw runtime_error ("match() expects at least 2 parameters");
			isExclude = p_isExclude;
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncReplace: public CFunctionCall {
	
	private:
		
		shared_ptr<CValue> replaceOne (shared_ptr<CValue> where, const string& what, const string& replacement);
	
	public:
		
		CFuncReplace (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 3)
				throw runtime_error ("replace() expects 3 parameters");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncRun: public CFunctionCall {
	
	private:
		
		bool captureOutput;
	
	public:
		
		CFuncRun (const vector<shared_ptr<CExpression>>& p_args, bool p_captureOutput): CFunctionCall (p_args) {
			if (args.size () < 1)
				throw runtime_error ("run() expects at least 1 parameter");
			captureOutput = p_captureOutput;
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncExists: public CFunctionCall {
	
	public:
		
		CFuncExists (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 1)
				throw runtime_error ("exists() expects 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncAbsPath: public CFunctionCall {
	
	public:
		
		CFuncAbsPath (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 1)
				throw runtime_error ("abspath() expects 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncRelPath: public CFunctionCall {
	
	private:
	
		string getRelativeTo (const string& fname, const string& relative_to);
	
	public:
		
		CFuncRelPath (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 1 && args.size () != 2)
				throw runtime_error ("relpath() expects 1 or 2 parameters");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncDirName: public CFunctionCall {
	
	public:
		
		CFuncDirName (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 1)
				throw runtime_error ("dirname() expects 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncFileName: public CFunctionCall {
	
	public:
		
		CFuncFileName (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 1)
				throw runtime_error ("filename() expects 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncCd: public CFunctionCall {
	
	public:
		
		CFuncCd (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 1)
				throw runtime_error ("cd() expects 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncCwd: public CFunctionCall {
	
	public:
		
		CFuncCwd (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 0)
				throw runtime_error ("cwd() expects 0 parameters");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncContains: public CFunctionCall {
	
	public:
		
		CFuncContains (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 2)
				throw runtime_error ("contains() expects 2 parameters");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncImplode: public CFunctionCall {
	
	public:
		
		CFuncImplode (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () < 1)
				throw runtime_error ("implode() expects at least 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncExplode: public CFunctionCall {
	
	public:
		
		CFuncExplode (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () < 1)
				throw runtime_error ("explode() expects at least 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncMkdir: public CFunctionCall {
	
	public:
		
		CFuncMkdir (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 1)
				throw runtime_error ("mkdir() expects 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncLick: public CFunctionCall {
	
	public:
		
		CFuncLick (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () < 1)
				throw runtime_error ("lick() expects at least 1 parameters");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncFail: public CFunctionCall {
	
	public:
		
		CFuncFail (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 1)
				throw runtime_error ("fail() expects 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncSubstr: public CFunctionCall {
	
	public:
		
		CFuncSubstr (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () < 2 || args.size() > 3)
				throw runtime_error ("substr() expects 2 or 3 parameters");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CFuncChr: public CFunctionCall {
	
	public:
		
		CFuncChr (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 1)
				throw runtime_error ("chr() expects 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
};

class CFuncOrd: public CFunctionCall {
	
	public:
		
		CFuncOrd (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 1)
				throw runtime_error ("ord() expects 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
};


class CFuncSep: public CFunctionCall {
	
	public:
		
		CFuncSep (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () > 1)
				throw runtime_error ("sep() expects at most 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
};

class CFuncCharAt: public CFunctionCall {
	
	public:
		
		CFuncCharAt (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () != 2)
				throw runtime_error ("char_at() expects 2 parameters");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
};

class CFuncHex: public CFunctionCall {
	
	public:
		
		CFuncHex (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () < 1 || args.size() > 2)
				throw runtime_error ("hex() expects 1 or 2 parameters");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
};


class CFuncDelete: public CFunctionCall {
	
	public:
		
		CFuncDelete (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () < 1)
				throw runtime_error ("delete() expect at least 1 parameter");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
};

class CFuncCopy: public CFunctionCall {
	
	public:
		
		CFuncCopy (const vector<shared_ptr<CExpression>>& p_args): CFunctionCall (p_args) {
			if (args.size () < 2)
				throw runtime_error ("copy() expects at least 2 parameters");
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
};


#endif /* __FUNCS_H__ */

