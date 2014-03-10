#include <stdexcept>

#include "module.h"
#include "sha1.h"
#include "hashstore.h"
#include "sys_funcs.h"
#include "stmt.h"

shared_ptr<CStatement> CStatement::parse (CInputParser& parser) {
	
	CToken token = parser.getToken ();
	
	if (token.getValue () == "{")
		return shared_ptr<CStatement> (new CCompoundStatement (parser));
	
	if (token.getValue () == "if")
		return shared_ptr<CStatement> (new CIfStatement (parser));

	if (token.getValue () == "for" || token.getValue () == "while")
		return shared_ptr<CStatement> (new CForStatement (token.getValue (), parser));

	if (token.getValue () == "break")
		return shared_ptr<CStatement> (new CBreakStatement (parser));
	
	if (token.getValue () == "continue")
		return shared_ptr<CStatement> (new CContinueStatement (parser));

	if (token.getValue () == "return")
		return shared_ptr<CStatement> (new CReturnStatement (parser));
	
	if (token.getValue () == "depends")
		return shared_ptr<CStatement> (new CDependsStatement (parser));

	if (token.getValue () == "include")
		return shared_ptr<CStatement> (new CIncludeStatement (parser));

	if (token.getValue () == "using")
		return shared_ptr<CStatement> (new CUsingStatement (parser));
	
	parser.pushBack (token);
	
	shared_ptr<CStatement> result = shared_ptr<CStatement> (new CExprStatement (parser));
	
	token = parser.getToken ();
	if (token.getValue () != ";")
		throw ESyntaxError (parser, "Expected ; at the end of statement");
	
	return result;
}

CStatement::CStatement (CInputParser& parser) {
	
	fileName = parser.getFileName ();
	lineNumber = parser.getLineNumber ();
	
}

void CStatement::execute (shared_ptr<CExecutionContext> ctx) {
	
	try {
		executeThrow (ctx);
	} catch (EExecutionError& e) {
		throw;
	} catch (exception& e) {
		throw EExecutionError ((*fileName), lineNumber, e.what ());
	} 
	
}

void CStatement::updateHash (shared_ptr<CExecutionContext> ctx, SHA1& hash) {

	if (this == NULL)
		hash.update ("null");
	else {
		hash.update ("class:");
		hash.update (typeid (*this).name());
		hash.update (":args:");	
		updateHashArgs (ctx, hash);
	}
}

CExprStatement::CExprStatement (CInputParser& parser): CStatement (parser) {
	expr = CExpression::parse (parser, 0);
}
		
void CExprStatement::executeThrow (shared_ptr<CExecutionContext> ctx) {
	expr -> evaluate (ctx);
}

void CExprStatement::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) {
	expr -> updateHash (ctx, hash);
}

CCompoundStatement::CCompoundStatement (CInputParser& parser): CStatement (parser)  {
	
	while (true) {
		
		CToken token = parser.getToken ();
		if (token.getValue () == "}")
			break;
		
		parser.pushBack (token);
		shared_ptr<CStatement> stmt = CStatement::parse (parser);
		stmts.push_back (stmt);
		
	}
	
}

void CCompoundStatement::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) {

	for (list<shared_ptr<CStatement>>::iterator it = stmts.begin(); it != stmts.end (); it++)
		(*it) -> updateHash (ctx, hash);
	
}

void CCompoundStatement::executeThrow (shared_ptr<CExecutionContext> ctx) {

	string oldCwd = getCurrentDirectory ();
	
	for (list<shared_ptr<CStatement>>::iterator it = stmts.begin(); it != stmts.end (); it++) {
		(*it) -> execute (ctx);
		if (ctx -> breakSignaled || ctx -> continueSignaled || ctx -> returnSignaled)
			break;
	}
	
	string newCwd = getCurrentDirectory ();
	if (newCwd != oldCwd) 
		setCurrentDirectory (oldCwd);
	
}

CDependsStatement::CDependsStatement (CInputParser& parser): CStatement (parser)  {

	CToken token = parser.getToken ();
	if (token.getValue () != "(")
		throw ESyntaxError (parser, "Expected (");
	
	expr = CExpression::parse (parser, 0);
	
	token = parser.getToken ();
	if (token.getValue () != ")")
		throw ESyntaxError (parser, "Expected )");
	
	actionStmt = CStatement::parse (parser);
	
}

void CDependsStatement::updateFileHash (SHA1& hash, const string& fileName) {

	string absName = getAbsolutePath (fileName);

	hash.update ("[name:[");
	hash.update (absName);
	hash.update ("]");
	
	long fsize = 0;
	time_t mtime = 0;
	
	if (getFileInfo (absName, &fsize, &mtime)) {
		
		stringstream ss;
		ss << ":exists:" << "size:" << fsize << ":time:" << mtime << ":";
		
		hash.update (ss.str());
		
	} else
		hash.update (":not exists:");
	
	hash.update ("]");
	
}

void CDependsStatement::executeThrow (shared_ptr<CExecutionContext> ctx) {

	shared_ptr<CValue> value = expr -> evaluate (ctx);

	SHA1 hash;
	hash.update ("depends:files[");
	
	if (value -> getType () == ValueArray) {
		for (int i = 0; i < value -> getLength (); i++) {
			shared_ptr<CValue> elem = value -> subscript (i);
			updateFileHash (hash, elem -> asString ());
		}
	} else
		updateFileHash (hash, value -> asString ());
	
	hash.update ("]");
	
	actionStmt -> updateHash (ctx, hash);
	
	string hashValue = hash.final ();
	
	if (!hashStore.containsHash (ctx -> getCurModule (), ctx -> getCurTarget (), hashValue)) {
		actionStmt -> execute (ctx);
		hashStore.addHash (ctx -> getCurModule (), ctx -> getCurTarget (), hashValue);
	}
	
}

void CDependsStatement::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) { 
	hash.update ("expr:"); expr -> updateHash (ctx, hash);
	hash.update ("stmt:"); actionStmt -> updateHash (ctx, hash);
}


CIfStatement::CIfStatement (CInputParser& parser): CStatement (parser)  {
	
	CToken token = parser.getToken ();
	if (token.getValue () != "(")
		throw ESyntaxError (parser, "( expected");
	
	expr = CExpression::parse (parser, 0);
	
	token = parser.getToken ();
	if (token.getValue () != ")")
		throw ESyntaxError (parser, ") expected");
	
	thenStmt = CStatement::parse (parser);
	
	token = parser.getToken ();
	if (token.getValue() == "else") {
		elseStmt = CStatement::parse (parser);
	} else
		parser.pushBack (token);
	
}

void CIfStatement::executeThrow (shared_ptr<CExecutionContext> ctx) {
	shared_ptr<CValue> value = expr -> evaluate (ctx);
	if (value -> asInt()) {
		thenStmt -> execute (ctx);
	} else {
		if (elseStmt)
			elseStmt -> execute (ctx);
	}
}

void CIfStatement::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) {
	hash.update ("expr:"); expr -> updateHash (ctx, hash);
	hash.update ("then:"); thenStmt -> updateHash (ctx, hash);
	if (elseStmt)
		hash.update ("else:"); elseStmt -> updateHash (ctx, hash);
}


CForStatement::CForStatement (const string& keyword, CInputParser& parser): CStatement (parser)  {

	CToken token = parser.getToken ();
	if (token.getValue () != "(")
		throw ESyntaxError (parser, "( expected");

	isForeach = false;

	if (keyword == "for") {
		
		// try foreach first
		
		CToken nameToken = parser.getToken ();

		if (nameToken.getTokenType () == NameToken) {
			CToken inToken = parser.getToken ();
			if (inToken.getValue () == "in") {
				isForeach = true;
				foreachVarName = nameToken.getValue ();
			} else {
				parser.pushBack (inToken);
				parser.pushBack (nameToken);
			}
		} else
			parser.pushBack (nameToken);
			
		if (isForeach) {

			initExpr = CExpression::parse (parser, 0);			
			
		} else {
			
			initExpr = CExpression::parse (parser, 0);
			
			token = parser.getToken ();
			if (token.getValue () != ";")
				throw ESyntaxError (parser, "; expected");

			whileExpr = CExpression::parse (parser, 0);
			
			token = parser.getToken ();
			if (token.getValue () != ";")
				throw ESyntaxError (parser, "; expected");

			nextExpr = CExpression::parse (parser, 0);
			
		}
		
	} else if (keyword == "while") {
		
		whileExpr = CExpression::parse (parser, 0);
		
	} else
		
		throw ESyntaxError (parser, "unknown loop keyword");
	
	token = parser.getToken ();
	if (token.getValue () != ")")
		throw ESyntaxError (parser, ") expected");
	
	loopStmt = CStatement::parse (parser);
	
}

void CForStatement::executeThrow (shared_ptr<CExecutionContext> ctx) {
	
	if (isForeach) {
		
		shared_ptr<CValue> arr = initExpr -> evaluate (ctx);
		
		if (arr -> getType () == ValueDict) {
			
			list<string> keys;
			arr -> getKeys (keys);
			
			for (list<string>::iterator it = keys.begin (); it != keys.end (); it++) {

				ctx -> getVarStore () -> setVar (foreachVarName, shared_ptr<CValue> (new CStringValue (*it)));
				loopStmt -> execute (ctx);
				
				if (ctx -> breakSignaled) {
					ctx -> breakSignaled = false;
					break;
				}
				
				if (ctx -> continueSignaled) 
					ctx -> continueSignaled = false;
				
				if (ctx -> returnSignaled)
					break;
				
			}
			
		} else if (arr -> getType () == ValueArray) {
			
			for (int index = 0; index < arr -> getLength(); index ++) {
				
				shared_ptr<CValue> elem = arr -> subscript (index);
				ctx -> getVarStore () -> setVar (foreachVarName, elem);
				loopStmt -> execute (ctx);
				
				if (ctx -> breakSignaled) {
					ctx -> breakSignaled = false;
					break;
				}
				
				if (ctx -> continueSignaled) 
					ctx -> continueSignaled = false;
				
				if (ctx -> returnSignaled)
					break;
				
			}
			
		} else {  // let's play nice and do one iteration for non-array type

			ctx -> getVarStore () -> setVar (foreachVarName, arr);
			loopStmt -> execute (ctx);
			ctx -> breakSignaled = false;
			ctx -> continueSignaled = false;
			
		}
		
	} else {
	
		if (initExpr)
			initExpr -> evaluate (ctx);

		while (true) {
			
			shared_ptr<CValue> result = whileExpr -> evaluate (ctx);
			if (result -> asInt() == 0)
				break;
			loopStmt -> execute (ctx);
			
			if (ctx -> breakSignaled) {
				ctx -> breakSignaled = false;
				break;
			}
			
			if (ctx -> continueSignaled) 
				ctx -> continueSignaled = false;
			
			if (ctx -> returnSignaled)
				break;
			
			if (nextExpr)
				nextExpr -> evaluate (ctx);
		}
		
	}
	
}

void CForStatement::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) {
	if (initExpr)
		hash.update ("init:"); initExpr -> updateHash (ctx, hash);
	if (whileExpr)
		hash.update ("while:"); whileExpr -> updateHash (ctx, hash);
	if (nextExpr)
		hash.update ("next:"); nextExpr -> updateHash (ctx, hash);
	hash.update ("loop:"); loopStmt -> updateHash (ctx, hash);
	
	if (isForeach) {
		hash.update ("foreach:");
		hash.update (foreachVarName);
	}
}


CBreakStatement::CBreakStatement (CInputParser& parser): CStatement (parser)  {

	CToken token = parser.getToken ();
	if (token.getValue () != ";")
		throw ESyntaxError (parser, "; expected");

}

void CBreakStatement::executeThrow (shared_ptr<CExecutionContext> ctx) {
	ctx -> breakSignaled = true;
}


CContinueStatement::CContinueStatement (CInputParser& parser): CStatement (parser)  {

	CToken token = parser.getToken ();
	if (token.getValue () != ";")
		throw ESyntaxError (parser, "; expected");

}

void CContinueStatement::executeThrow (shared_ptr<CExecutionContext> ctx) {
	ctx -> continueSignaled = true;
}

CReturnStatement::CReturnStatement (CInputParser& parser): CStatement (parser)  {

	CToken token = parser.getToken ();
	if (token.getValue () != ";") {
		parser.pushBack (token);
		expr = CExpression::parse (parser, 0);
		
		token = parser.getToken ();
		if (token.getValue () != ";") 
			throw ESyntaxError (parser, "; expected");
		
	}
	
}

void CReturnStatement::executeThrow (shared_ptr<CExecutionContext> ctx) {
	
	shared_ptr<CValue> retValue = (expr.get() != nullptr) ? expr -> evaluate (ctx) : shared_ptr<CValue> (new CVoidValue ());

	ctx -> returnValue (retValue);
	
}

void CReturnStatement::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) { 
	if (expr)
		expr -> updateHash (ctx, hash);
}

CIncludeStatement::CIncludeStatement (CInputParser& parser): CStatement (parser)  {
	
	expr = CExpression::parse (parser, 0);
	
	CToken token = parser.getToken ();
	if (token.getValue () != ";") 
		throw ESyntaxError (parser, "; expected");
	
}

void CIncludeStatement::executeThrow (shared_ptr<CExecutionContext> ctx) {
	
	string includePath = getAbsolutePath (makeSysSeparators (expr -> evaluate (ctx) -> asString ())); 
	
	if (!ctx -> getBaseContext () -> hasIncludedModule (includePath)) {
	
		CLineCountedInputFile input (includePath);
		CInputParser parser (input);

		CModule includedModule (parser);
		
		ctx -> getBaseContext () -> addIncludedModule (includePath);

		includedModule.addFunctionsToContext (ctx, true);
		includedModule.execute (ctx);	
		
	}
}

void CIncludeStatement::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) { 
	expr -> updateHash (ctx, hash);
}

CUsingStatement::CUsingStatement (CInputParser& parser): CStatement (parser)  {
	
	expr = CExpression::parse (parser, 0);
	
	CToken token = parser.getToken ();
	if (token.getValue () != ";") 
		throw ESyntaxError (parser, "; expected");
	
}

void CUsingStatement::executeThrow (shared_ptr<CExecutionContext> ctx) {
	
	string usingPath = getAbsolutePath (makeSysSeparators (expr -> evaluate (ctx) -> asString ())); 
	
	if (!ctx -> getBaseContext () -> hasIncludedModule (usingPath)) {
		
		hashStore.setNameFromModule (usingPath);
		
		size_t pos = usingPath.find_last_of (getAnyPathSeparator ());
		string dirName = usingPath.substr (0, pos);
		
		string oldCwd = getCurrentDirectory ();
		setCurrentDirectory (dirName);
	
		CLineCountedInputFile input (usingPath);
		CInputParser parser (input);

		CModule includedModule (parser);
		
		ctx -> getBaseContext () -> addIncludedModule (usingPath);

		includedModule.addFunctionsToContext (ctx, false);
		includedModule.execute (ctx);	
		
		setCurrentDirectory (oldCwd);
		
	}
}

void CUsingStatement::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) { 
	expr -> updateHash (ctx, hash);
}
