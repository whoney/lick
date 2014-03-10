#ifndef __PARSER_H__
#define __PARSER_H__

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <stdexcept>
#include <memory>

using namespace std;

class CLineCountedInputFile {

	private:
		
		shared_ptr<string> fname;
		
		ifstream ifs;
		int lineCount;
		string pushbackBuffer;
		
	public:
		
		CLineCountedInputFile (const string& fileName);
		~CLineCountedInputFile ();
		
		bool eof ();
		char get ();
		void unget (char c);
		
		int getLineNumber () {
			return lineCount;
		}
		
		shared_ptr<string> getFileName () {
			return fname;
		}
	
};

enum TokenType {
	
	StringLiteral,
	NumLiteral,
	NameToken,
	Punctuation,
	Operation,
	EndOfFile
	
};

class CToken {

	private:

		TokenType tokenType;
		string value;
	
	public:
		
		CToken () {
			tokenType = EndOfFile;
			value = "";
		}
		
		CToken (const TokenType p_tokenType, const string& p_value) {
			tokenType = p_tokenType;
			value = p_value;
		}
		
		CToken (const TokenType p_tokenType, const char p_value) {
			tokenType = p_tokenType;
			value.push_back (p_value);
		}
		
		CToken (const CToken& rhs) {
			tokenType = rhs.tokenType;
			value = rhs.value;
		}
		
		CToken& operator= (const CToken& rhs) {
			tokenType = rhs.tokenType;
			value = rhs.value;
			return *this;
		}		
		
		TokenType getTokenType () {
			return tokenType;
		}
		
		string getValue () {
			return value;
		}
		
};


class CInputParser {
	
	private:
	
		CLineCountedInputFile& input;
		list<CToken> pushbackBuffer;
		CToken lastToken;

		CToken getTokenRaw ();
		
	public:
		
		CInputParser (CLineCountedInputFile& pInput): input (pInput) { }
		
		CToken getToken ();
		void pushBack (CToken& token);
		
		string getLocation ();
		string getLastToken ();
		
		shared_ptr<string> getFileName ();
		int getLineNumber ();
	
};

class ESyntaxError: public runtime_error {
	
	public:
		
		ESyntaxError (CInputParser& parser, const string& msg): runtime_error("[" + parser.getLocation() + "]: " + msg + " (got '" + parser.getLastToken () + "')") { }
	
};



#endif /* __PARSER_H__ */
