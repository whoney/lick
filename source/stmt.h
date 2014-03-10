#ifndef __STMT_H__
#define __STMT_H__

#include <memory>

class CStatement;

#include "parser.h"
#include "expr.h"
#include "context.h"
#include "sha1.h"

using namespace std;

class EExecutionError: public runtime_error {
	
	private:
		
		string mWhat;
		
	public:
		
		EExecutionError (const string& p_fileName, int p_lineNumber, const string& p_msg): runtime_error (p_msg) {
			stringstream ss;
			ss << "[" << p_fileName << ":" << (p_lineNumber+1) << "]: " << p_msg;
			mWhat = ss.str();
		}
			
		virtual const char* what() const throw() {
			return mWhat.c_str();
		}
	
};



class CStatement {
	
	private:
		
		shared_ptr<string> fileName;
		int lineNumber;
	
	protected:
	
		virtual void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) = 0;
		virtual void executeThrow (shared_ptr<CExecutionContext> ctx) = 0;
		
	public:
		
		CStatement (CInputParser& parser);
		
		static shared_ptr<CStatement> parse (CInputParser& parser);

		void updateHash (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		void execute (shared_ptr<CExecutionContext> ctx);
	
};

class CExprStatement: public CStatement {

	private:
		
		shared_ptr<CExpression> expr;

	protected:
		
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		void executeThrow (shared_ptr<CExecutionContext> ctx);
		
	public:
		
		CExprStatement (CInputParser& parser);
		
	
};

class CCompoundStatement: public CStatement {
	
	private:
		
		list<shared_ptr<CStatement>> stmts;
		
	protected:
		
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		void executeThrow (shared_ptr<CExecutionContext> ctx);
		
	public:
		
		CCompoundStatement (CInputParser& parser);
	
};

class CIfStatement: public CStatement {

	private:
		
		shared_ptr<CExpression> expr;
		shared_ptr<CStatement> thenStmt;
		shared_ptr<CStatement> elseStmt;

	protected:
	
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		void executeThrow (shared_ptr<CExecutionContext> ctx);
		
	public:
		
		CIfStatement (CInputParser& parser);
	
};

class CForStatement: public CStatement {

	private:
		
		shared_ptr<CExpression> initExpr;
		shared_ptr<CExpression> whileExpr;
		shared_ptr<CExpression> nextExpr;
		shared_ptr<CStatement> loopStmt;
		
		bool isForeach;
		string foreachVarName;

	protected:
	
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		void executeThrow (shared_ptr<CExecutionContext> ctx);
		
	public:
		
		CForStatement (const string& keyword, CInputParser& parser);
	
};

class CBreakStatement: public CStatement {

	protected:
	
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) { }
		void executeThrow (shared_ptr<CExecutionContext> ctx);
	
	public:
	
		CBreakStatement (CInputParser& parser);
	
};


class CContinueStatement: public CStatement {

	protected:
	
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) { }
		void executeThrow (shared_ptr<CExecutionContext> ctx);
		
	public:
	
		CContinueStatement (CInputParser& parser);
	
};

class CReturnStatement: public CStatement {
	
	private:
	
		shared_ptr<CExpression> expr;

	protected:
	
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		void executeThrow (shared_ptr<CExecutionContext> ctx);
		
	public:
	
		CReturnStatement (CInputParser& parser);
	
};

class CIncludeStatement: public CStatement {
	
	private:
	
		shared_ptr<CExpression> expr;

	protected:
	
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		void executeThrow (shared_ptr<CExecutionContext> ctx);
		
	public:
	
		CIncludeStatement (CInputParser& parser);
	
};

class CUsingStatement: public CStatement {
	
	private:
	
		shared_ptr<CExpression> expr;

	protected:
	
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		void executeThrow (shared_ptr<CExecutionContext> ctx);
		
	public:
	
		CUsingStatement (CInputParser& parser);
	
};

class CDependsStatement: public CStatement {

	private:
		
		shared_ptr<CExpression> expr;
		shared_ptr<CStatement> actionStmt;
		
		void updateFileHash (SHA1& hash, const string& fileName);
		
	protected:
	
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		void executeThrow (shared_ptr<CExecutionContext> ctx);
		
	public:
		
		CDependsStatement (CInputParser& parser);
	
};

#endif /* __STMT_H__ */
