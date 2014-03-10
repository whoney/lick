#include <stdexcept>
#include <cstdlib>

#include "parser.h"
#include "expr.h"
#include "funcs.h"

map<string,OpCode> initBinaryOps () {
	map<string,OpCode> result;
	
	result["="] = OpAssign;
	result["&&"] = OpLogicAnd;
	result["||"] = OpLogicOr;
	result["+"] = OpAdd; 
	result["-"] = OpSubtract;
	result["/"] = OpDivide;
	result["*"] = OpMultiply;
	result["["] = OpSubscript;
	result["=="] = OpEqual;
	result["!="] = OpNotEqual;
	result["<"] = OpLess;
	result["<="] = OpLessOrEqual;
	result[">"] = OpMore;
	result[">="] = OpMoreOrEqual;

	return result;
}

map<string,OpCode> CExpression::binaryOps = initBinaryOps ();

map<string,OpCode> initUnaryOps () {
	map<string,OpCode> result;

	result["-"] = OpUnaryMinus; 
	result["~"] = OpBitwiseNot;
	result["!"] = OpNot;
	result["++"] = OpIncrement;
	result["--"] = OpDecrement;

	return result;
}

map<string,OpCode> CExpression::unaryOps = initUnaryOps ();

map<string,OpCode> initPostfixOps () {
	map<string,OpCode> result;

	result["++"] = OpIncrement;
	result["--"] = OpDecrement;

	return result;
}


map<string,OpCode> CExpression::postfixOps = initPostfixOps ();

void CExpression::updateHash (shared_ptr<CExecutionContext> ctx, SHA1& hash) {

	if (this == NULL)
		hash.update ("null");
	else {
		hash.update ("class:");
		hash.update (typeid (*this).name());
		hash.update (":args:");	
		updateHashArgs (ctx, hash);
	}
	
}

shared_ptr<CExpression> CExpression::parseOne (CInputParser& parser) {
	
	shared_ptr<CExpression> expr = parseBase (parser);
	
	CToken token = parser.getToken ();
	map<string,OpCode>::iterator it = postfixOps.find (token.getValue ());
	
	if (it != postfixOps.end ()) {
		return shared_ptr<CExpression> (new CUnaryOperation (expr, it -> second, true));		
	} else {
		parser.pushBack (token);
		return expr;
	}

}

shared_ptr<CExpression> CExpression::parseBase (CInputParser& parser) {
	
	CToken token = parser.getToken ();
	
	switch (token.getTokenType ()) {
		
		case NumLiteral:
			return shared_ptr<CExpression> (new CConstantExpression (shared_ptr<CValue> (new CIntValue (strtol (token.getValue().c_str(), NULL, 0)))));
			
		case StringLiteral:
			return shared_ptr<CExpression> (new CConstantExpression (shared_ptr<CValue> (new CStringValue (token.getValue ()))));
			
		case NameToken:
		{
			CToken next = parser.getToken ();
			if (next.getValue() == "(") { // function call
				
				vector<shared_ptr<CExpression>> args;
				while (true) {
					
					CToken arg = parser.getToken ();
					if (arg.getValue () == ")")
						break;
					parser.pushBack (arg);
					
					shared_ptr<CExpression> expr = parse (parser, 0);
					
					args.push_back (expr);
					
					arg = parser.getToken ();
					if (arg.getValue () == ")")
						break;
					else if (arg.getValue () != ",")
						throw ESyntaxError (parser, "Expected ) or ,");
				}
				
				shared_ptr<CExpression> fcall (CFunctionCall::makeFunctionCall (token.getValue (), args));
				
				return fcall;
				
			} else {
				parser.pushBack (next);
				return shared_ptr<CExpression> (new CVarRefExpression (token.getValue ()));
			}
		}
			
		case Operation:
		{
				map<string,OpCode>::iterator it = unaryOps.find (token.getValue ());

				if (it != unaryOps.end ()) {
					return shared_ptr<CExpression> (new CUnaryOperation (parseOne (parser), it -> second, false));
				} else
					throw ESyntaxError (parser, "Not an unary operation");
		}

		case Punctuation:
			if (token.getValue() == "(") {
				
				shared_ptr<CExpression> expr = parse (parser, 0);
				CToken closingBracket = parser.getToken ();
				if (closingBracket.getValue() != ")")
					throw ESyntaxError (parser, "Expected )");
				
				return expr;
			
			} else if (token.getValue() == "[") {	// array literal, so well so good
				
				CArrayExpression *arrayExpr = new CArrayExpression ();
				
				while (true) {
					CToken next = parser.getToken ();
					if (next.getValue() == "]")
						break;
					
					parser.pushBack (next);
					shared_ptr<CExpression> expr = parse (parser, 0);
					arrayExpr -> append (expr);

					next = parser.getToken ();
					if (next.getValue() == "]")
						break;
					else if (next.getValue () != ",")
						throw ESyntaxError (parser, "Expected , or ]");					
				}
				
				return shared_ptr<CExpression> (arrayExpr);
				
			} else if (token.getValue() == "{") { // dict literal
				
				CDictExpression *dictExpr = new CDictExpression ();
				
				while (true) {
					CToken next = parser.getToken ();
					if (next.getValue() == "}")
						break;
					
					shared_ptr<CExpression> keyExpr;
					
					if (next.getValue() == ".") { // dot form
						CToken keyName = parser.getToken ();
						if (keyName.getTokenType () != NameToken)
							throw ESyntaxError (parser, "Expected key name");
						
						keyExpr = shared_ptr<CExpression> (new CConstantExpression (shared_ptr<CValue> (new CStringValue (keyName.getValue ()))));
						
					} else {
					
						parser.pushBack (next);
						keyExpr = parse (parser, 0);
						
					}

					next = parser.getToken ();
					if (next.getValue() != ":")
						throw ESyntaxError (parser, "Expected :");
					
					shared_ptr<CExpression> valueExpr = parse (parser, 0);
					dictExpr -> append (keyExpr, valueExpr);
					
					next = parser.getToken ();
					if (next.getValue() == "}")
						break;
					else if (next.getValue () != ",")
						throw ESyntaxError (parser, "Expected , or }");					
				}
				
				return shared_ptr<CExpression> (dictExpr);
				
			} else
				throw ESyntaxError (parser, "Unexpected token");
			
		default:
			throw ESyntaxError (parser, "Unexpected token");
			
	}
	
}

shared_ptr<CExpression> CExpression::parse (CInputParser& parser, int precedenceLevel) {
	
	shared_ptr<CExpression> lhs = parseOne (parser);
	
	while (true) {
	
		CToken token = parser.getToken ();
		
		switch (token.getTokenType ()) {

			case Operation:
			case Punctuation:
			{
				
				if (token.getValue () == ".") {
					
					CToken nameToken = parser.getToken ();
					if (nameToken.getTokenType () == NameToken) {
						
						CToken leftBracket = parser.getToken ();
						if (leftBracket.getValue () == "(") {
							
							vector<shared_ptr<CExpression>> args;
							args.push_back (lhs);
							
							while (true) {
								
								CToken arg = parser.getToken ();
								if (arg.getValue () == ")")
									break;
								parser.pushBack (arg);
								
								shared_ptr<CExpression> expr = parse (parser, 0);
								
								args.push_back (expr);
								
								arg = parser.getToken ();
								if (arg.getValue () == ")")
									break;
								else if (arg.getValue () != ",")
									throw ESyntaxError (parser, "Expects ) or ,");
							}
							
							lhs = CFunctionCall::makeFunctionCall (nameToken.getValue (), args);
							break;
							
						} else {
							
							parser.pushBack (leftBracket);
							
							shared_ptr<CExpression> rhs = shared_ptr<CExpression> (new CConstantExpression (shared_ptr<CValue> (new CStringValue (nameToken.getValue()))));
							lhs = shared_ptr<CExpression> (new CBinaryOperation (lhs, rhs, OpSubscript)); 

							break;
							
						}
						
					}
					
					parser.pushBack (nameToken);
					parser.pushBack (token);
					return lhs;
					
				} else if (token.getValue() == "?") {
					
					shared_ptr<CExpression> trueExpr = parse (parser, 0);					
					CToken colon = parser.getToken ();
					if (colon.getValue () != ":") 
						throw ESyntaxError (parser, "Expects :");

					shared_ptr<CExpression> falseExpr = parse (parser, 0);
					lhs = shared_ptr<CExpression> (new CTernaryOperation (lhs, trueExpr, falseExpr));
				
				} else {
				
					map<string,OpCode>::iterator it = binaryOps.find (token.getValue ());
					bool withAssignment = false;
					
					if (it == binaryOps.end ()) {
						if (token.getValue () != "") {
							if (token.getValue ().back() == '=') {
								string withoutEqual = token.getValue().substr (0, token.getValue().length() - 1);
								it = binaryOps.find (withoutEqual);
								withAssignment = true;
							}
						}
					}

					if (it != binaryOps.end ()) {
						
						if (it -> second < precedenceLevel) {
							parser.pushBack (token);
							return lhs;
						} else {
							shared_ptr<CExpression> rhs = parse (parser, it -> second);
							shared_ptr<CExpression> new_lhs = shared_ptr<CExpression> (new CBinaryOperation (lhs, rhs, it -> second));
							if (it -> second == OpSubscript) {
								CToken next = parser.getToken ();
								if (next.getValue () != "]")
									throw ESyntaxError (parser, "Expected ] at the end of subscript");								
							}
							
							if (withAssignment) 
								new_lhs = shared_ptr <CExpression> (new CBinaryOperation (lhs, new_lhs, OpAssign));
							
							lhs = new_lhs;
						}
						
					} else {
						parser.pushBack (token);
						return lhs;
					}
					
				}
				
				break;
			}
			
			default:
				parser.pushBack (token);
				return lhs;
			
		}
		
	}

}

shared_ptr<CValue> CUnaryOperation::evaluate (shared_ptr<CExecutionContext> ctx) {

	if (op == OpIncrement || op == OpDecrement) {	// prefix
		shared_ptr<CValueRef> ref = arg -> evalRef (ctx);
		shared_ptr<CValue> oldValue = ref -> getValue ();
		
		int intValue = oldValue -> asInt ();
		if (op == OpIncrement)
			intValue ++;
		else
			intValue --;
		
		shared_ptr<CValue> newValue (new CIntValue (intValue));
		ref -> setValue (shared_ptr<CValue> (newValue));
		return postfix ? oldValue : newValue;
	}
	
	shared_ptr<CValue> value = arg -> evaluate (ctx);
	
	switch (op) {
		case OpUnaryMinus:
			return shared_ptr<CValue> (new CIntValue (- value -> asInt()));
		case OpBitwiseNot:
			return shared_ptr<CValue> (new CIntValue (~ value -> asInt()));
		case OpNot:
			return shared_ptr<CValue> (new CIntValue (! value -> asInt()));
		default:
			throw runtime_error ("Failed to evaluate");
	}
	
}

void CUnaryOperation::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) {
	stringstream ss;
	ss << (postfix ? "postfix":"prefix");
	ss << op << ":";
	hash.update (ss.str());
	arg -> updateHash (ctx, hash);
}

void CTernaryOperation::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) {
	cond -> updateHash (ctx, hash);
	trueExpr -> updateHash (ctx, hash);
	falseExpr -> updateHash (ctx, hash);
}

shared_ptr<CValue> CTernaryOperation::evaluate (shared_ptr<CExecutionContext> ctx) {
	
	int condValue = cond -> evaluate (ctx) -> asInt ();
	if (condValue)
		return trueExpr -> evaluate (ctx);
	else
		return falseExpr -> evaluate (ctx);

}

shared_ptr<CValueRef> CBinaryOperation::evalRef (shared_ptr<CExecutionContext> ctx) {

	if (op == OpSubscript) {
		
		shared_ptr<CValue> left = lhs -> evaluate (ctx);
		
		if (left -> getType () == ValueDict) {
			string index = rhs -> evaluate (ctx) -> asString ();
			shared_ptr<CValueRef> entry = left -> subscriptRef (index);
			return entry;
		} else {
			int index = rhs -> evaluate (ctx) -> asInt ();
			shared_ptr<CValueRef> entry = left -> subscriptRef (index);
			return entry;
		}
		
	} else
		return CExpression::evalRef (ctx);

}

shared_ptr<CValue> CBinaryOperation::evaluate (shared_ptr<CExecutionContext> ctx) {
	
	if (op == OpAssign) {
		
		shared_ptr<CValueRef> left = lhs -> evalRef (ctx);
		shared_ptr<CValue> right = rhs -> evaluate (ctx);
		
		left -> setValue (right);
		return right;
		
	} else if (op == OpSubscript) {
		
		shared_ptr<CValue> left = lhs -> evaluate (ctx);
		
		if (left -> getType () == ValueDict) {
			string index = rhs -> evaluate (ctx) -> asString ();
			shared_ptr<CValue> entry = left -> subscript (index);
			return entry;
		} else {
			int index = rhs -> evaluate (ctx) -> asInt ();
			shared_ptr<CValue> entry = left -> subscript (index);
			return entry;
		}
		
	} else {
	
		shared_ptr<CValue> left = lhs -> evaluate (ctx);
		shared_ptr<CValue> right = rhs -> evaluate (ctx);
		
		switch (op) {
			case OpAdd:
			{
				if (left -> getType () == ValueVoid)
					return right;
				
				if (right -> getType () == ValueVoid)
					return left;
				
				if (left -> getType () == ValueArray && right -> getType () == ValueArray)
					return shared_ptr<CValue> (CArrayValue::make_concat (left, right));

				if (left -> getType () == ValueArray)
					return shared_ptr<CValue> (CArrayValue::make_append (left, right));

				if (right -> getType () == ValueArray)
					return shared_ptr<CValue> (CArrayValue::make_prepend (left, right));
				
				if (left -> getType () == ValueString || right -> getType() == ValueString)
					return shared_ptr<CValue> (new CStringValue (left -> asString() + right -> asString()));
				else
					return shared_ptr<CValue> (new CIntValue (left -> asInt() + right -> asInt()));
			}
			
			case OpEqual:
			case OpNotEqual:
			{
				bool isEqual;
				
				if (left -> getType () == ValueInt && right -> getType() == ValueInt)
					isEqual = (left -> asInt() == right -> asInt ());
				else
					isEqual = (left -> asString() == right -> asString ());
				
				return shared_ptr<CValue> (new CIntValue ((op == OpEqual) ? isEqual : (!isEqual)));
			}
			
			case OpLess:
			case OpMoreOrEqual:
			{
				bool isLess;
				
				if (left -> getType () == ValueInt && right -> getType() == ValueInt)
					isLess = (left -> asInt() < right -> asInt ());
				else
					isLess = (left -> asString() < right -> asString ());
				
				return shared_ptr<CValue> (new CIntValue ((op == OpLess) ? isLess : (!isLess)));
			}

			case OpMore:
			case OpLessOrEqual:
			{
				bool isMore;
				
				if (left -> getType () == ValueInt && right -> getType() == ValueInt)
					isMore = (left -> asInt() > right -> asInt ());
				else
					isMore = (left -> asString() > right -> asString ());
				
				return shared_ptr<CValue> (new CIntValue ((op == OpMore) ? isMore : (!isMore)));
			}
			
			case OpSubtract:
				return shared_ptr<CValue> (new CIntValue (left -> asInt() - right -> asInt()));

			case OpDivide:
				return shared_ptr<CValue> (new CIntValue (left -> asInt() / right -> asInt()));

			case OpMultiply:
				return shared_ptr<CValue> (new CIntValue (left -> asInt() * right -> asInt()));
				
			case OpLogicAnd:
				return shared_ptr<CValue> (new CIntValue (left -> asInt() && right -> asInt()));

			case OpLogicOr:
				return shared_ptr<CValue> (new CIntValue (left -> asInt() || right -> asInt()));

			default:
				throw runtime_error ("Failed to evaluate");
		}
		
	}
	
}

void CBinaryOperation::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) {
	stringstream ss;
	ss << op << ":";
	hash.update (ss.str());
	hash.update (":lhs:");
	lhs -> updateHash (ctx, hash);
	hash.update (":rhs:");
	rhs -> updateHash (ctx, hash);
}

shared_ptr<CValue> CArrayExpression::evaluate (shared_ptr<CExecutionContext> ctx) {

	CArrayValue *value = new CArrayValue ();
	
	for (list<shared_ptr<CExpression>>::iterator it = elems.begin (); it != elems.end (); it++) {
		shared_ptr<CValue> elem = (*it) -> evaluate (ctx);
		shared_ptr<CValueRef> ref (new CValueRef (elem));
		value -> append (ref);
	}
	
	return shared_ptr<CValue> (value);
	
}

void CArrayExpression::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) {
	for (list<shared_ptr<CExpression>>::iterator it = elems.begin (); it != elems.end (); it++) {
		hash.update (":elem:");
		(*it) -> updateHash (ctx, hash);
	}
}

shared_ptr<CValue> CDictExpression::evaluate (shared_ptr<CExecutionContext> ctx) {

	CDictValue *dict = new CDictValue ();

	for (map<shared_ptr<CExpression>,shared_ptr<CExpression>>::iterator it = elems.begin (); it != elems.end (); it++) {
		shared_ptr<CValue> key = it -> first -> evaluate (ctx);
		shared_ptr<CValue> value = it -> second -> evaluate (ctx);
		shared_ptr<CValueRef> ref (new CValueRef (value));
		dict -> append (key -> asString (), ref);
	}
	
	return shared_ptr<CValue> (dict);
	
}

void CDictExpression::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) {
	
	for (map<shared_ptr<CExpression>,shared_ptr<CExpression>>::iterator it = elems.begin (); it != elems.end (); it++) {
		hash.update (":key:");
		it -> first -> updateHash (ctx, hash);
		hash.update (":value:");
		it -> second -> updateHash (ctx, hash);
	}
}

void CConstantExpression::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) {
	value -> updateHash (hash);
}

void CVarRefExpression::updateHashArgs (shared_ptr<CExecutionContext> ctx, SHA1& hash) {
	hash.update (":name:");
	hash.update (varName);
	hash.update (":value:");
	shared_ptr<CValue> value = ctx -> getVarStore () -> getVar (varName);
	value -> updateHash (hash);
}
