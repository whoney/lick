#include <map>
#include <stdexcept>
#include <sstream>
#include <fstream>
#include <iomanip>

#include "module.h"
#include "sys_funcs.h"
#include "funcs.h"

enum BuiltinFunc {

	funcStrlen,
	funcLength,
	funcReplace,
	funcPrint,
	funcPrintln,
	funcFiles,
	funcMatch,
	funcExclude,
	funcReadfile,
	funcWritefile,
	funcRun,
	funcCapture,
	funcExists,
	funcAbsPath,
	funcCd,
	funcContains,
	funcCwd,
	funcRelpath,
	funcDirname,
	funcImplode,
	funcExplode,
	funcMkdir,
	funcLick,
	funcFail,
	funcSubstr,
	funcChr,
	funcOrd,
	funcSep,
	funcCharAt,
	funcHex,
	funcDelete,
	funcFilename,
	funcCopy
	
};

map<string, BuiltinFunc> initBuiltinFuncs () {
	map<string, BuiltinFunc> result;

	result["strlen"] = funcStrlen ;
	result["length"] = funcLength ;
	result["replace"] = funcReplace ;
	result["print"] = funcPrint ;
	result["println"] = funcPrintln ;
	result["files"] = funcFiles ;
	result["match"] = funcMatch ;
	result["exclude"] = funcExclude ;
	result["readfile"] = funcReadfile ;
	result["writefile"] = funcWritefile ;
	result["run"] = funcRun ;
	result["capture"] = funcCapture ;
	result["exists"] = funcExists ;
	result["abspath"] = funcAbsPath ;
	result["cd"] = funcCd ;
	result["contains"] = funcContains ;
	result["cwd"] = funcCwd ;
	result["relpath"] = funcRelpath ;
	result["dirname"] = funcDirname ;
	result["filename"] = funcFilename ;
	result["implode"] = funcImplode ;
	result["explode"] = funcExplode ;
	result["mkdir"] = funcMkdir ;
	result["lick"] = funcLick ;
	result["fail"] = funcFail ;
	result["substr"] = funcSubstr ;
	result["chr"] = funcChr ;
	result["ord"] = funcOrd ;
	result["sep"] = funcSep ;
	result["char_at"] = funcCharAt ;
	result["hex"] = funcHex ;
	result["delete"] = funcDelete ;
	result["copy"] = funcCopy ;
	
	return result;
}

static map<string, BuiltinFunc> builtinFuncs = initBuiltinFuncs ();

shared_ptr<CFunctionCall> CFunctionCall::getBuiltinFunction (const string& name, const vector<shared_ptr<CExpression>>& args) {

	map<string, BuiltinFunc>::iterator it = builtinFuncs.find (name);
	if (it == builtinFuncs.end ())
		return shared_ptr<CFunctionCall> ();
	
	switch (it -> second) {
	
		case funcStrlen:
			return shared_ptr<CFunctionCall> (new CFuncStrlen (args));
			
		case funcLength:
			return shared_ptr<CFunctionCall> (new CFuncLength (args));

		case funcPrint:
			return shared_ptr<CFunctionCall> (new CFuncPrint (args, false));
			
		case funcPrintln:
			return shared_ptr<CFunctionCall> (new CFuncPrint (args, true));

		case funcFiles:
			return shared_ptr<CFunctionCall> (new CFuncFiles (args));
			
		case funcMatch:
			return shared_ptr<CFunctionCall> (new CFuncMatch (args, false));

		case funcExclude:
			return shared_ptr<CFunctionCall> (new CFuncMatch (args, true));
			
		case funcReadfile:
			return shared_ptr<CFunctionCall> (new CFuncReadFile (args));

		case funcWritefile:
			return shared_ptr<CFunctionCall> (new CFuncWriteFile (args));
			
		case funcReplace:
			return shared_ptr<CFunctionCall> (new CFuncReplace (args));
			
		case funcRun:
			return shared_ptr<CFunctionCall> (new CFuncRun (args, false));

		case funcCapture:
			return shared_ptr<CFunctionCall> (new CFuncRun (args, true));
			
		case funcExists:
			return shared_ptr<CFunctionCall> (new CFuncExists (args));

		case funcAbsPath:
			return shared_ptr<CFunctionCall> (new CFuncAbsPath (args));

		case funcRelpath:
			return shared_ptr<CFunctionCall> (new CFuncRelPath (args));

		case funcDirname:
			return shared_ptr<CFunctionCall> (new CFuncDirName (args));

		case funcFilename:
			return shared_ptr<CFunctionCall> (new CFuncFileName (args));

		case funcCd:
			return shared_ptr<CFunctionCall> (new CFuncCd (args));

		case funcCwd:
			return shared_ptr<CFunctionCall> (new CFuncCwd (args));

		case funcContains:
			return shared_ptr<CFunctionCall> (new CFuncContains (args));
			
		case funcImplode:
			return shared_ptr<CFunctionCall> (new CFuncImplode (args));

		case funcExplode:
			return shared_ptr<CFunctionCall> (new CFuncExplode (args));

		case funcMkdir:
			return shared_ptr<CFunctionCall> (new CFuncMkdir (args));

		case funcLick:
			return shared_ptr<CFunctionCall> (new CFuncLick (args));
			
		case funcFail:
			return shared_ptr<CFunctionCall> (new CFuncFail (args));

		case funcSubstr:
			return shared_ptr<CFunctionCall> (new CFuncSubstr (args));

		case funcChr:
			return shared_ptr<CFunctionCall> (new CFuncChr (args));

		case funcOrd:
			return shared_ptr<CFunctionCall> (new CFuncOrd (args));

		case funcSep:
			return shared_ptr<CFunctionCall> (new CFuncSep (args));

		case funcCharAt:
			return shared_ptr<CFunctionCall> (new CFuncCharAt (args));

		case funcHex:
			return shared_ptr<CFunctionCall> (new CFuncHex (args));

		case funcDelete:
			return shared_ptr<CFunctionCall> (new CFuncDelete (args));

		case funcCopy:
			return shared_ptr<CFunctionCall> (new CFuncCopy (args));

		default:
			return shared_ptr<CFunctionCall> ();
		
	}
	
}

shared_ptr<CFunctionCall> CFunctionCall::makeFunctionCall (const string& name, const vector<shared_ptr<CExpression>>& args) {

	shared_ptr<CFunctionCall> builtin = getBuiltinFunction (name, args);
	if (builtin)
		return builtin;
	
	return shared_ptr<CFunctionCall> (new CUserFunctionCall (args, name));
}


shared_ptr<CValue> CUserFunctionCall::evaluate (shared_ptr<CExecutionContext> ctx) {
	
	shared_ptr<CUserFunction> func = ctx -> getBaseContext () -> getFunction (userFuncName);
	return func -> execute (ctx, args);
	
}

void CFunctionCall::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) {

	for (vector<shared_ptr<CExpression>>::iterator it = args.begin (); it != args.end (); it++) {
		hash.update (":arg:");
		(*it) -> updateHash (ctx, hash);
	}
	
}

void CUserFunctionCall::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) {
	
	hash.update (":name:");
	hash.update (userFuncName);
	
	hash.update (":func:");
	shared_ptr<CUserFunction> func = ctx -> getBaseContext () -> getFunction (userFuncName);
	func -> updateHash (ctx, hash);
	
	CFunctionCall::updateHashArgs (ctx, hash);
}

shared_ptr<CValue> CFuncFiles::evaluate (shared_ptr<CExecutionContext> ctx) {
	shared_ptr<CValue> arg = args[0] -> evaluate (ctx);
	
	string path = arg -> asString ();
	if (path.empty ())
		path = ".";
	
	list<string> files = getFilesInPath (path);

	CArrayValue *result = new CArrayValue ();
	for (list<string>::iterator it = files.begin (); it != files.end (); it++)
		result -> append (shared_ptr<CValueRef> (new CValueRef (shared_ptr<CValue> (new CStringValue (*it)))));
	
	return shared_ptr<CValue> (result);
	
}

static int glob (const char *c, const char *s) {

	const char *here;

	for (;;) {

		switch( *c++ ) {

			case '\0':
				return *s ? -1 : 0;

			case '?':
				if( !*s++ )
					return 1;
				break;


			case '*':
				here = s;

				while( *s ) 
					s++;

				/* Try to match the rest of the pattern in a recursive */
				/* call.  If the match fails we'll back up chars, retrying. */

				while( s != here )
				{
					int r;

					/* A fast path for the last token in a pattern */

					r = *c ? glob( c, s ) : *s ? -1 : 0;

					if( !r )
						return 0;
					else if( r < 0 )
						return 1;

					--s;
				}
				break;

			case '\\':
				/* Force literal match of next char. */

				if( !*c || *s++ != *c++ )
					return 1;
				break;

			default:
				if( *s++ != c[-1] )
					return 1;
				break;
		}
	}
}



bool CFuncMatch::testMatch (const string& value, const string& pattern) {

	return (glob (pattern.c_str(), value.c_str ()) == 0);
	
}

bool CFuncMatch::matchOne (shared_ptr<CExecutionContext> ctx, shared_ptr<CValue> value) {
	
	for (size_t i = 1; i < args.size (); i++) {
		shared_ptr<CValue> arg = args[i] -> evaluate (ctx);
		string pattern = arg -> asString ();
		if (testMatch (value -> asString (), pattern))
			return isExclude ? false : true;
	}
	
	return isExclude ? true : false;
	
}

shared_ptr<CValue> CFuncMatch::evaluate (shared_ptr<CExecutionContext> ctx) {
	
	shared_ptr<CValue> strings = args[0] -> evaluate (ctx);
	
	if (strings -> getType () == ValueArray) {
		CArrayValue *result = new CArrayValue ();
		for (int i = 0; i < strings -> getLength (); i++) {
			shared_ptr<CValue> item = strings -> subscript (i);
			if (matchOne (ctx, item))
				result -> append (shared_ptr<CValueRef> (new CValueRef (item)));
		}
		return shared_ptr<CValue> (result);
	} else
		return shared_ptr<CValue> (new CIntValue (matchOne (ctx, strings) ? 1 : 0));
		

}

shared_ptr<CValue> CFuncReadFile::evaluate (shared_ptr<CExecutionContext> ctx) {
	
	string fname = args[0] -> evaluate (ctx) -> asString ();
	
	ifstream ifs;
	
	ifs.open (makeSysSeparators(fname).c_str(), ifstream::binary);
	if (!ifs.is_open ()) {
		throw runtime_error ("File does not exist");
	}
	
	// string result ((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());	
	
	stringstream buffer;
	buffer << ifs.rdbuf();
	ifs.close ();
	
	string result = buffer.str (); 
	
	return shared_ptr<CValue> (new CStringValue (result));
	
}

shared_ptr<CValue> CFuncWriteFile::evaluate (shared_ptr<CExecutionContext> ctx) {
	
	string fname = args[0] -> evaluate (ctx) -> asString ();
	string content = args[1] -> evaluate (ctx) -> asString ();
	
	ofstream ofs;
	
	ofs.open (makeSysSeparators(fname).c_str());
	if (!ofs.is_open ()) {
		throw runtime_error ("File does not exist");
	}
	
	stringstream buffer;
	buffer << content;
	ofs << buffer.rdbuf ();
	ofs.close ();
	
	return shared_ptr<CValue> (new CVoidValue ());
	
}

shared_ptr<CValue> CFuncReplace::replaceOne (shared_ptr<CValue> where, const string& what, const string& replacement) {

	string in = where -> asString ();
	
	size_t pos;
	while ((pos = in.find (what)) != string::npos) 
		in = in.replace (pos, what.length(), replacement);
	
	return shared_ptr<CValue> (new CStringValue (in));
}


shared_ptr<CValue> CFuncReplace::evaluate (shared_ptr<CExecutionContext> ctx) {

	shared_ptr<CValue> where = args[0] -> evaluate (ctx);
	
	string what = args[1] -> evaluate (ctx) -> asString ();
	string replacement = args[2] -> evaluate (ctx) -> asString ();
	
	if (where -> getType () == ValueArray) {
		
		CArrayValue *result = new CArrayValue ();
		
		for (int i = 0; i < where -> getLength (); i++) {
			shared_ptr<CValue> newValue = replaceOne (where -> subscript (i), what, replacement);
			result -> append (shared_ptr<CValueRef> (new CValueRef (newValue)));
		}
		
		return shared_ptr<CValue> (result);
		
	} else
		return replaceOne (where, what, replacement);

}

shared_ptr<CValue> CFuncRun::evaluate (shared_ptr<CExecutionContext> ctx) {

	list<string> params;
	
	for (vector<shared_ptr<CExpression>>::const_iterator it = args.begin (); it != args.end (); it ++) {
	
		shared_ptr<CValue> value = (*it) -> evaluate (ctx);
		if (value -> getType () == ValueArray) {
			for (int i = 0; i < value -> getLength (); i++) {
				shared_ptr<CValue> elem = value -> subscript (i);
				string s = elem -> asString ();
				if (!s.empty ())
					params.push_back (s);
			}
		} else {
			string s = value -> asString ();
			if (!s.empty ()) {
			
				size_t pos = 0;
				while (pos < s.length ()) {
					size_t nonSpace = s.find_first_not_of (" \t", pos);
					if (nonSpace == string::npos)
						break;
					
					pos = nonSpace;
					size_t nextSpace = s.find_first_of (" \t", pos);
					if (nextSpace == string::npos)
						nextSpace = s.length ();
						
					string p = s.substr (pos, nextSpace - pos);
					params.push_back (p);

					pos = nextSpace;
				}
			}
		}
	
	}
	
	map<string,string> envmap;
	bool hasEnv = false;
	
	shared_ptr<CValue> envVar = ctx -> getVarStore () -> getVar ("sys");
	if (envVar -> getType() == ValueDict) {
		shared_ptr<CValue> envDict = envVar -> subscript ("env");
		if (envDict -> getType () == ValueDict) {
			
			hasEnv = true;
			
			list<string> envKeys;
			envDict -> getKeys (envKeys);
			
			for (list<string>::iterator it = envKeys.begin (); it != envKeys.end (); it++) {
				string key = *it;
				string value = envDict -> subscript (key) -> asString ();
				envmap [key] = value;
			}
			
			shared_ptr<CValue> pathArray = envVar -> subscript ("path");
			if (pathArray -> getType () == ValueArray) {
				string pathValue = "";
				for (int i = 0; i < pathArray -> getLength (); i++) {
					string pathComp = pathArray -> subscript (i) -> asString ();
					if (!pathValue.empty ())
						pathValue.push_back (ENV_PATH_SEPARATOR);
					pathValue += pathComp;
				}
				
				envmap ["PATH"] = pathValue;
			}
		
		}
	}
	
	string capture_stdout;
	
	int retCode = runCommand (params, captureOutput ? (&capture_stdout) : NULL, hasEnv ? &envmap : NULL);
	
	if (retCode != 0) 
		throw runtime_error ("Command exec failed");
		
	return captureOutput ? shared_ptr<CValue>(new CStringValue (capture_stdout)) : shared_ptr<CValue>(new CVoidValue ());

}

shared_ptr<CValue> CFuncExists::evaluate (shared_ptr<CExecutionContext> ctx) {

	shared_ptr<CValue> arg = args[0] -> evaluate (ctx);;
	string fname = arg -> asString ();
	
	if (getFileInfo (fname, NULL, NULL))
		return shared_ptr<CValue> (new CIntValue (1));
	else
		return shared_ptr<CValue> (new CIntValue (0));
	
}

shared_ptr<CValue> CFuncAbsPath::evaluate (shared_ptr<CExecutionContext> ctx) {

	shared_ptr<CValue> arg = args[0] -> evaluate (ctx);;
	
	if (arg -> getType () == ValueArray) {
		CArrayValue *arr = new CArrayValue ();
		
		for (int i = 0; i < arg -> getLength(); i++) {
			shared_ptr<CValue> elem = arg -> subscript (i);
			shared_ptr<CValue> resElem (new CStringValue (getAbsolutePath (elem -> asString())));
			arr -> append (shared_ptr<CValueRef> (new CValueRef (resElem)));
		}
		
		return shared_ptr<CArrayValue> (arr);
		
	} else {
		return shared_ptr<CValue> (new CStringValue (getAbsolutePath (arg -> asString())));
	}
	
}

string CFuncRelPath::getRelativeTo (const string& fname, const string& relative_to) {

	char sep_char = getPathSeparator() [0];
	size_t cpos;
	
	for (cpos = 0; cpos < fname.length() && cpos < relative_to.length (); cpos ++) {
		if (fname.at (cpos) != relative_to.at (cpos))
			break;
	}
		
	if (cpos == 0) 
		return fname;
		
	if (cpos >= relative_to.length ()) {
		if (cpos >= fname.length())
			return ".";
			
		if (cpos < fname.length() && fname.at (cpos) == sep_char) {
			return fname.substr (cpos + 1, string::npos);
		}
	}
	
	do {
		cpos --;
		if (fname.at(cpos) == sep_char && relative_to.at (cpos) == sep_char)
			break;
	} while (cpos > 0);
	
	if (cpos == 0) 
		return fname;

	int rel_pop_components = 0;
	for (size_t rel_sp = cpos; rel_sp < relative_to.length (); rel_sp ++)
		if (relative_to.at (rel_sp) == sep_char)
			rel_pop_components ++;
	
	string result;
	for (int i = 0; i < rel_pop_components; i++)
		result += ".." + getPathSeparator ();
		
	result += fname.substr (cpos+1, string::npos);
	return result;
}

shared_ptr<CValue> CFuncRelPath::evaluate (shared_ptr<CExecutionContext> ctx) {

	shared_ptr<CValue> arg = args[0] -> evaluate (ctx);;
	string relative_to = (args.size () >= 2) ? getAbsolutePath (makeSysSeparators (args[1] -> evaluate (ctx) -> asString ())) : makeSysSeparators (getCurrentDirectory ());
	
	if (arg -> getType () == ValueArray) {
		CArrayValue *arr = new CArrayValue ();

		for (int i = 0; i < arg -> getLength(); i++) {
			shared_ptr<CValue> elem = arg -> subscript (i);
			shared_ptr<CValue> resElem (new CStringValue (getRelativeTo (getAbsolutePath (makeSysSeparators (elem -> asString())), relative_to)));
			arr -> append (shared_ptr<CValueRef> (new CValueRef (resElem)));
		}
		
		return shared_ptr<CArrayValue> (arr);

	} else
		return shared_ptr<CValue> (new CStringValue (getRelativeTo (getAbsolutePath (makeSysSeparators (arg -> asString())), relative_to)));

}

shared_ptr<CValue> CFuncDirName::evaluate (shared_ptr<CExecutionContext> ctx) {

	string fname = args[0] -> evaluate (ctx) -> asString ();
	size_t pos = fname.find_last_of (getAnyPathSeparator ());
	
	if (pos == string::npos)
		return shared_ptr<CValue> (new CStringValue ("."));
	else {
		return shared_ptr<CValue> (new CStringValue (fname.substr (0, pos)));
	}
}

shared_ptr<CValue> CFuncFileName::evaluate (shared_ptr<CExecutionContext> ctx) {

	string fname = args[0] -> evaluate (ctx) -> asString ();
	return shared_ptr<CValue> (new CStringValue (extractFileName (fname)));
	
}


shared_ptr<CValue> CFuncCd::evaluate (shared_ptr<CExecutionContext> ctx) {

	shared_ptr<CValue> arg = args[0] -> evaluate (ctx);
	
	setCurrentDirectory (arg -> asString ());

	return shared_ptr<CValue> (new CStringValue (getCurrentDirectory ()));
}

shared_ptr<CValue> CFuncCwd::evaluate (shared_ptr<CExecutionContext> ctx) {
	return shared_ptr<CValue> (new CStringValue (getCurrentDirectory ()));
}

shared_ptr<CValue> CFuncContains::evaluate (shared_ptr<CExecutionContext> ctx) {

	shared_ptr<CValue> container = args[0] -> evaluate (ctx);
	shared_ptr<CValue> what = args[1] -> evaluate (ctx);

	if (container -> getType() == ValueArray) {
		for (int i = 0; i < container -> getLength (); i++) {
			shared_ptr<CValue> elem = container -> subscript (i);
			if (elem -> asString() == what -> asString ())
				return shared_ptr<CValue> (new CIntValue (1));
		}
	} else {
		string s = container -> asString ();
		if (s.find (what -> asString ()) != string::npos)
			return shared_ptr<CValue> (new CIntValue (1));
	}

	return shared_ptr<CValue> (new CIntValue (0));

}

shared_ptr<CValue> CFuncImplode::evaluate (shared_ptr<CExecutionContext> ctx) {

	shared_ptr<CValue> arr = args[0] -> evaluate (ctx);
	string sep;

	if (args.size() > 1)
		sep = args[1] -> evaluate (ctx) -> asString ();
	else
		sep = " ";

	if (arr -> getType() == ValueArray) {
		string result;
		bool first = true;
		for (int i = 0; i < arr -> getLength (); i++) {
			shared_ptr<CValue> elem = arr -> subscript (i);
			if (!first)
				result += sep;
			else
				first = false;
			result += elem -> asString ();
		}
		return shared_ptr<CValue> (new CStringValue (result));
	} else {
		return arr;
	}

}

shared_ptr<CValue> CFuncExplode::evaluate (shared_ptr<CExecutionContext> ctx) {

	string str = args[0] -> evaluate (ctx) -> asString ();	
	string seps;

	if (args.size () > 1)
		seps = args[1] -> evaluate (ctx) -> asString ();
	else
		seps = " \n\r\t";
	
	CArrayValue *arr = new CArrayValue ();
	
	size_t pos = 0;
	while (pos < str.length ()) {
		pos = str.find_first_not_of (seps, pos);
		if (pos == string::npos)
			break;
		size_t nextSep = str.find_first_of (seps, pos);
		if (nextSep == string::npos)
			nextSep = str.length ();
			
		string s = str.substr (pos, nextSep - pos);
		arr -> append (shared_ptr<CValueRef> (new CValueRef (shared_ptr<CValue> (new CStringValue (s)))));
		
		pos = nextSep;
	}
	

	return shared_ptr<CValue> (arr);
}


shared_ptr<CValue> CFuncMkdir::evaluate (shared_ptr<CExecutionContext> ctx) {

	string path = args[0] -> evaluate (ctx) -> asString ();
	makeDirs (makeSysSeparators (path));

	return shared_ptr<CValue> (new CVoidValue ());

}

shared_ptr<CValue> CFuncLick::evaluate (shared_ptr<CExecutionContext> ctx) {

	string path = getAbsolutePath (makeSysSeparators (args[0] -> evaluate (ctx) -> asString ()));
	string target = "";
	
	if (args.size() >= 2)
		target = args[1] -> evaluate (ctx) -> asString ();
	
	list<string> params;
	
	for (size_t i = 2; i < args.size (); i++) {
		string param = args[i] -> evaluate (ctx) -> asString ();
		params.push_back (param);
	}
	
	if (isDirectory (path))
		path += getPathSeparator () + "lickable";
	
	size_t pos = path.find_last_of (getAnyPathSeparator ());
	string dirName = path.substr (0, pos);
	
	string oldCwd = getCurrentDirectory ();
	setCurrentDirectory (dirName);
	
	CLineCountedInputFile input (path);
	CInputParser parser (input);
	CModule module (parser);
	
	shared_ptr<CExecutionContext> newCtx (new CExecutionContext (path));
	
	newCtx -> getBaseContext () -> addIncludedModule (path);
	
	module.addFunctionsToContext (newCtx, true);
	module.execute (newCtx);
	
	if (target.empty ()) {
		if (module.hasTarget ("default"))
			target = "default";
		else if (module.hasTarget ("all"))
			target = "all";
	}
	
	shared_ptr<CValue> retValue;
	
	if (!target.empty ()) 
		retValue = module.executeTarget (newCtx, target, params);
	else
		retValue = shared_ptr<CValue> (new CVoidValue ());
	
	setCurrentDirectory (oldCwd);

	return retValue;

}

shared_ptr<CValue> CFuncFail::evaluate (shared_ptr<CExecutionContext> ctx) {

	string reason = args[0] -> evaluate (ctx) -> asString ();
	throw runtime_error (reason);

}

shared_ptr<CValue> CFuncSubstr::evaluate (shared_ptr<CExecutionContext> ctx) {

	string s = args[0] -> evaluate (ctx) -> asString ();
	size_t startPos = args[1] -> evaluate (ctx) -> asInt ();
	size_t len = (args.size () > 2) ? (args[2] -> evaluate (ctx) -> asInt ()) : string::npos;

	string result = s.substr (startPos, len);

	return shared_ptr<CValue> (new CStringValue (result));

}

shared_ptr<CValue> CFuncCharAt::evaluate (shared_ptr<CExecutionContext> ctx) {

	string s = args[0] -> evaluate (ctx) -> asString ();
	size_t pos = args[1] -> evaluate (ctx) -> asInt ();
	string result = s.substr (pos, 1);

	return shared_ptr<CValue> (new CStringValue (result));

}

shared_ptr<CValue> CFuncChr::evaluate (shared_ptr<CExecutionContext> ctx) {

	char c = (char) (args[0] -> evaluate (ctx) -> asInt ());
	string s;

	s.push_back (c);
	
	return shared_ptr<CValue> (new CStringValue (s));
}

shared_ptr<CValue> CFuncOrd::evaluate (shared_ptr<CExecutionContext> ctx) {

	string s = args[0] -> evaluate (ctx) -> asString ();
	int result = 0;
	if (s.length() > 0) 
		result = ((int) (s.at (0))) & 0xFF;
	
	return shared_ptr<CValue> (new CIntValue (result));
}

shared_ptr<CValue> CFuncHex::evaluate (shared_ptr<CExecutionContext> ctx) {

	stringstream ss;

	ss << hex;

	int value = args[0] -> evaluate (ctx) -> asInt ();
	
	if (args.size () > 1) {
		int padding = args[1] -> evaluate (ctx) -> asInt ();
		ss << setw (padding) << setfill ('0');
	}
	
	ss << value;
	
	return shared_ptr<CValue> (new CStringValue (ss.str()));
}

shared_ptr<CValue> CFuncSep::evaluate (shared_ptr<CExecutionContext> ctx) {

	if (args.size () == 0)
		return shared_ptr<CValue> (new CStringValue (getPathSeparator ()));

	shared_ptr<CValue> where = args[0] -> evaluate (ctx);
	
	if (where -> getType () == ValueArray) {
		
		CArrayValue *result = new CArrayValue ();
		
		for (int i = 0; i < where -> getLength (); i++) {
			shared_ptr<CValue> newValue = shared_ptr<CValue> (new CStringValue (makeSysSeparators (where -> subscript (i) -> asString ())));
			result -> append (shared_ptr<CValueRef> (new CValueRef (newValue)));
		}
		
		return shared_ptr<CValue> (result);
		
	} else
		return shared_ptr<CValue> (new CStringValue (makeSysSeparators (where -> asString ())));

}



shared_ptr<CValue> CFuncDelete::evaluate (shared_ptr<CExecutionContext> ctx) {

	for (vector<shared_ptr<CExpression>>::iterator it = args.begin (); it != args.end (); it++) {
	
		shared_ptr<CValue> arg = (*it) -> evaluate (ctx);
		
		if (arg -> getType () == ValueArray) {
			for (int i = 0; i < arg -> getLength (); i++)
				deleteFileOrDir (makeSysSeparators (arg -> subscript (i) -> asString ()));
		
		} else
			deleteFileOrDir (makeSysSeparators (arg -> asString ()));
	
	}
	
	return shared_ptr<CValue> (new CVoidValue ());

}

shared_ptr<CValue> CFuncCopy::evaluate (shared_ptr<CExecutionContext> ctx) {

	string copy_to = makeSysSeparators (args[args.size()-1] -> evaluate (ctx) -> asString ());
	
	for (size_t i = 0; i < args.size() - 1; i++) {
	
		shared_ptr<CValue> arg = args[i] -> evaluate (ctx);
		
		if (arg -> getType () == ValueArray) {
			for (int j = 0; j < arg -> getLength (); j++)
				copyFile (makeSysSeparators (arg -> subscript (j) -> asString ()), copy_to);
		
		} else
			copyFile (makeSysSeparators (arg -> asString ()), copy_to);
	
	}
	
	return shared_ptr<CValue> (new CVoidValue ());

}


