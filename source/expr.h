#ifndef __EXPR_H__
#define __EXPR_H__

#include <memory>
#include <map>

class CExpression;

#include "value.h"
#include "context.h"
#include "sha1.h"

using namespace std;

enum OpCode {	// order here determines precedence
	OpAssign,
	OpLogicAnd,
	OpLogicOr,
	OpEqual,
	OpNotEqual,
	OpLess,
	OpLessOrEqual,
	OpMore,
	OpMoreOrEqual,
	OpAdd,
	OpSubtract,
	OpDivide,
	OpMultiply,
	OpIncrement,
	OpDecrement,
	OpSubscript,
	OpUnaryMinus,
	OpBitwiseNot,
	OpNot
};

class CExpression {

		static shared_ptr<CExpression> parseOne (CInputParser& parser);
		static shared_ptr<CExpression> parseBase (CInputParser& parser);

		static map<string,OpCode> binaryOps;
		static map<string,OpCode> unaryOps;
		static map<string,OpCode> postfixOps;
		
	protected:
	
		virtual void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) = 0;

	public:
		
		virtual shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx) = 0;
		
		virtual shared_ptr<CValueRef> evalRef (shared_ptr<CExecutionContext> ctx) {
			throw ERuntimeError ("Expression is not an lvalue");
		}
		
		static shared_ptr<CExpression> parse (CInputParser& parser, int precedenceLevel);
		
		void updateHash (shared_ptr<CExecutionContext> ctx, SHA1& hash);
	
};

class CConstantExpression: public CExpression {
	
	private:
		
		shared_ptr<CValue> value;

	protected:
	
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		
	public:
		
		CConstantExpression (shared_ptr<CValue> p_value) {
			value = p_value;
		}
		
		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx) {
			return value;
		}
	
};

class CVarRefExpression: public CExpression {
	
	private:
		
		string varName;
		
	protected:
	
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		
	public:
		
		CVarRefExpression (const string& p_varName): varName (p_varName) { }
		
		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx) {
			return ctx -> getVarStore () -> getVar (varName);
		}
		
		shared_ptr<CValueRef> evalRef (shared_ptr<CExecutionContext> ctx) {
			return ctx -> getVarStore () -> getVarRef (varName);
		}
	
};

class CUnaryOperation: public CExpression {
	
	private:
		
		shared_ptr<CExpression> arg;
		OpCode op;
		bool postfix;
		
	protected:
	
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		
	public:
		
		CUnaryOperation (shared_ptr<CExpression> p_arg, OpCode p_op, bool p_postfix) :
			arg (p_arg),
			op (p_op), 
			postfix (p_postfix) { }
	
		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);

};


class CBinaryOperation: public CExpression {
	
	private:
		
		shared_ptr<CExpression> lhs;
		shared_ptr<CExpression> rhs;
		OpCode op;
		
	protected:
	
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		
	public:
		
		CBinaryOperation (shared_ptr<CExpression> p_lhs, shared_ptr<CExpression> p_rhs, OpCode p_op) :
			lhs (p_lhs),
			rhs (p_rhs), 
			op (p_op) { }
	
		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		shared_ptr<CValueRef> evalRef (shared_ptr<CExecutionContext> ctx);

};

class CTernaryOperation: public CExpression {

	private:
		
		shared_ptr<CExpression> cond;
		shared_ptr<CExpression> trueExpr;
		shared_ptr<CExpression> falseExpr;
		
	protected:
	
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		
	public:
		
		CTernaryOperation (shared_ptr<CExpression> p_cond, shared_ptr<CExpression> p_trueExpr, shared_ptr<CExpression> p_falseExpr) :
			cond (p_cond),
			trueExpr (p_trueExpr),
			falseExpr (p_falseExpr) { }
	
		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
	
};

class CArrayExpression: public CExpression {
	
	private:
		
		list<shared_ptr<CExpression>> elems;
		
	protected:
	
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		
	public:
		
		void append (shared_ptr<CExpression> expr) {
			elems.push_back (expr);
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

class CDictExpression: public CExpression {
	
	private:
		
		map<shared_ptr<CExpression>,shared_ptr<CExpression>> elems;

	protected:
	
		void updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash);
		
	public:
		
		void append (shared_ptr<CExpression> key, shared_ptr<CExpression> value) {
			elems.insert (pair<shared_ptr<CExpression>,shared_ptr<CExpression>> (key, value));
		}

		shared_ptr<CValue> evaluate (shared_ptr<CExecutionContext> ctx);
		
};

#endif /* __EXPR_H__ */

