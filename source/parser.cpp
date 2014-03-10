#include <stdexcept>
#include <cctype>
#include <cstdlib>

#include "parser.h"
#include "sys_funcs.h"

CLineCountedInputFile::CLineCountedInputFile (const string& fileName) {

	lineCount = 0;
	fname = shared_ptr<string> (new string (getAbsolutePath (fileName)));
	
	ifs.open (fileName.c_str(), ifstream::in);
	
	if (!ifs.is_open ())
		throw runtime_error ("Cannot open " + fileName);
		
}

CLineCountedInputFile::~CLineCountedInputFile () {
	if (ifs.is_open ())
		ifs.close ();
}

bool CLineCountedInputFile::eof () {

	if (pushbackBuffer.empty ())
		return ifs.eof ();
	else
		return false;
	
}

char CLineCountedInputFile::get () {
	
	if (!pushbackBuffer.empty ()) {
		char c = pushbackBuffer.at (pushbackBuffer.length () - 1);
		pushbackBuffer.resize (pushbackBuffer.length () - 1);
		return c;
	} 
	
	char c = ifs.get ();
	if (c == '\n')
		lineCount ++;
	
	return c;	
}

void CLineCountedInputFile::unget (char c) {
	pushbackBuffer.push_back (c);
}

void CInputParser::pushBack (CToken& token) {
	pushbackBuffer.push_back (token);
}

CToken CInputParser::getToken () {
	CToken result = getTokenRaw ();
	lastToken = result;
	return result;
}

string CInputParser::getLastToken () {
	return lastToken.getValue ();
}

CToken CInputParser::getTokenRaw () {
	
	if (!pushbackBuffer.empty ()) {
		CToken result = pushbackBuffer.back ();
		pushbackBuffer.pop_back ();
		return result;
	}

	char c;
	
	while (true) {
	
		do {
			if (input.eof ())
				return CToken (EndOfFile, "");
			c = input.get();
		} while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
		
		if (c == EOF) {
			return CToken (EndOfFile, "");
		}
		
		if (c == '/') {
			char next = input.get ();
			if (next == '/') {
				
				while (!input.eof ()) {
					c = input.get ();
					if (c == '\n' || c == '\r')
						break;
				} 
				
			} else if (next == '*') {
				
				while (!input.eof ()) {
					c = input.get ();
					if (c == '*') {
						next = input.get ();
						if (next == '/')
							break;
						else
							input.unget (next);
					}
				}
				
			} else {
				input.unget (next);
				break;
			}
		} else
			break;
	}
	
	if (c == '"') {
		
		string stringLiteral;
		
		while (!input.eof ()) {
			c = input.get ();
			if (c == '"') {
					break;
			}
			
			if (c == '\\') {
				char c2 = input.get ();
				switch (c2) {
					case 'n':	stringLiteral.push_back ('\n'); break;
					case 'r':	stringLiteral.push_back ('\r'); break;
					case 't':	stringLiteral.push_back ('\t'); break;
					default:	stringLiteral.push_back (c2); break;
				}
			} else
				stringLiteral.push_back (c);
		}
		
		return CToken (StringLiteral, stringLiteral);
		
	} else if (isdigit (c)) {
		
		string numLiteral;
		numLiteral.push_back (c);
		bool isHex = false;

		while (!input.eof ()) {
			c = input.get ();
			
			if (c == 'x' && numLiteral == "0") {
				isHex = true;
				numLiteral.push_back (c);
				continue;
			}
			
			if (!(isHex?isxdigit(c):isdigit (c))) {
				input.unget (c);
				break;
			}
			
			numLiteral.push_back (c);
		}
		
		return CToken (NumLiteral, numLiteral);
		
	} else if (isalpha (c) || c == '_') {
		
		string name;
		
		name.push_back (c);
		
		while (!input.eof ()) {
			c = input.get ();
			
			if (!isalnum (c) && c != '_') {
				input.unget (c);
				break;
			}
			
			name.push_back (c);
		}
		
		return CToken (NameToken, name);
		
	} else {
		
		string punct = "()[]{};,.";
		string strictOps = "~?:";
		string singleOps = "/*%^!";	// may be followed with '='
		string doubleOps = "+-=&|<>";	// may be doubled and optionally followed with '=', but not ++= neither --= nor ===
		
		if (punct.find (c) != string::npos)
			return CToken (Punctuation, c);
		
		if (strictOps.find (c) != string::npos)
			return CToken (Operation, c);
		
		if (singleOps.find (c) != string::npos) {

			char next = input.get ();
			if (next == '=') {
				
				string opCode;
				opCode.push_back (c);
				opCode.push_back (next);
				
				return CToken (Operation, opCode);
			} else {
				
				input.unget (next);
				return CToken (Operation, c);
			}
			
		}
		
		if (doubleOps.find (c) != string::npos) {

			string opCode;
			opCode.push_back (c);
			
			char next = input.get ();
			if (next == c) {
				
				opCode.push_back (c);
				if (opCode == "++" || opCode == "--" || opCode == "==")	// no such thing as "++="
					return CToken (Operation, opCode);
			} else
				input.unget (next);
			
			char maybeEqual = input.get ();
			
			if (maybeEqual == '=') {
				opCode.push_back (maybeEqual);
				return CToken (Operation, opCode);
			} else
				input.unget (maybeEqual);
			
			return CToken (Operation, opCode);
			
		}
		
		throw ESyntaxError (*this, "Syntax error");
		
	}
	
}

string CInputParser::getLocation () {

	stringstream ss;
	ss << (*input.getFileName ()) << ":" << (input.getLineNumber ()+1);
	return ss.str();
	
}

shared_ptr<string> CInputParser::getFileName () {
	return input.getFileName ();
}

int CInputParser::getLineNumber () {
	return input.getLineNumber ();
}
