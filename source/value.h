#ifndef __VALUE_H__
#define __VALUE_H__

#include <string>
#include <cstdio>
#include <memory>
#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>

#include "sys_funcs.h"
#include "sha1.h"

using namespace std;

class ERuntimeError: public runtime_error {
	
	public:
		
		ERuntimeError (const string& msg): runtime_error(msg) { }
	
};

enum ValueType {
	ValueVoid,
	ValueInt,
	ValueString,
	ValueArray,
	ValueDict
};

class CValueRef;

class CValue {
	
	protected:
	
		virtual void updateHashArgs (SHA1& hash) = 0;
	
	public:
		
		virtual int asInt () = 0;
		virtual string asString () = 0;
		virtual ValueType getType () = 0;
		
		virtual shared_ptr<CValue> subscript (int index) {
			throw ERuntimeError ("Cannot subscript");
		}
		
		virtual shared_ptr<CValueRef> subscriptRef (int index) {
			throw ERuntimeError ("Cannot subscript");
		}
		
		virtual shared_ptr<CValue> subscript (const string& index) {
			throw ERuntimeError ("Cannot subscript");
		}
		
		virtual shared_ptr<CValueRef> subscriptRef (const string& index) {
			throw ERuntimeError ("Cannot subscript");
		}
		
		virtual void getKeys (list<string>& keys) {
			throw ERuntimeError ("Cannot get keys");
		}
		
		virtual int getLength () {
			return 0;
		}
		
		void updateHash (SHA1& hash);

};

class CValueRef: public CValue {
	
	private:
		
		shared_ptr<CValue> value;
		
	protected:
		
		void updateHashArgs (SHA1& hash) {
			value -> updateHash (hash);
		}
		
	public:
		
		CValueRef (shared_ptr<CValue> p_value): value(p_value) { }
		
		int asInt () {
			return value -> asInt ();
		}
		
		string asString () {
			return value -> asString ();
		}

		ValueType getType () {
			return value -> getType ();
		}
		
		shared_ptr<CValue> getValue () {
			return value;
		}
		
		void setValue (shared_ptr<CValue> p_value) {
			value = p_value;
		}
	
};

class CVoidValue: public CValue {
	
	protected:
		
		void updateHashArgs (SHA1& hash) { }
	
	public:
		
		int asInt () {
			return 0;
		}
		
		string asString () {
			return string ("");
		}

		ValueType getType () {
			return ValueVoid;
		}
		
};

class CArrayValue: public CValue {
	
	protected:
		
		void updateHashArgs (SHA1& hash) {
			for (vector<shared_ptr<CValueRef>>::iterator it = values.begin(); it != values.end(); it++) {
				hash.update ("ref:"); 
				(*it) -> updateHash (hash);
			}
		}
	
	private:
		
		vector<shared_ptr<CValueRef>> values;
		
	public:
		
		int asInt () {
			return 0;
		}
		
		string asString () {
			stringstream ss;
			ss << "[";
			bool first = true;
			
			for (vector<shared_ptr<CValueRef>>::iterator it = values.begin(); it != values.end(); it++) {
				if (!first)
					ss << ", ";
				else
					first = false;
				
				ss << "\"" << (*it) -> getValue () -> asString () << "\"";
				
			}
			ss << "]";
			return ss.str();
		}

		ValueType getType () {
			return ValueArray;
		}
		
		void append (shared_ptr<CValueRef> value) {
			values.push_back (value);
		}
		
		static CArrayValue *make_concat (shared_ptr<CValue> array1, shared_ptr<CValue> array2) {
		
			CArrayValue *result = new CArrayValue ();
			for (int i = 0; i < array1 -> getLength (); i++)
				result -> append (shared_ptr<CValueRef> (new CValueRef (array1 -> subscript (i))));

			for (int i = 0; i < array2 -> getLength (); i++)
				result -> append (shared_ptr<CValueRef> (new CValueRef (array2 -> subscript (i))));
			
			return result;
		}
		
		static CArrayValue *make_append (shared_ptr<CValue> arr, shared_ptr<CValue> val) {
		
			CArrayValue *result = new CArrayValue ();
			for (int i = 0; i < arr -> getLength (); i++)
				result -> append (shared_ptr<CValueRef> (new CValueRef (arr -> subscript (i))));
			
			result -> append (shared_ptr<CValueRef> (new CValueRef (val)));

			return result;
		}

		static CArrayValue *make_prepend (shared_ptr<CValue> val, shared_ptr<CValue> arr) {
		
			CArrayValue *result = new CArrayValue ();

			result -> append (shared_ptr<CValueRef> (new CValueRef (val)));

			for (int i = 0; i < arr -> getLength (); i++)
				result -> append (shared_ptr<CValueRef> (new CValueRef (arr -> subscript (i))));
			

			return result;
		}
		
		shared_ptr<CValue> subscript (int index) {
			if (index >= 0) {

				unsigned uIndex = (unsigned) index;
				
				if (uIndex < values.size()) {
					shared_ptr<CValueRef> ref = values[uIndex];
					return ref -> getValue ();
				} else
					return shared_ptr<CValue> (new CVoidValue ());
			} else
				return shared_ptr<CValue> (new CVoidValue ());
		}
		
		shared_ptr<CValueRef> subscriptRef (int index) {
			if (index >= 0) {
				
				unsigned uIndex = (unsigned) index;
				
				while (uIndex >= values.size ()) {
					shared_ptr<CValue> voidValue (new CVoidValue());
					shared_ptr<CValueRef> voidRef (new CValueRef (voidValue));
					values.push_back (voidRef);
				}

				return values[uIndex];
				
			} else
				throw ERuntimeError ("Array index out of range");
		}
		
		int getLength () {
			return values.size ();
		}
	
};

class CDictValue: public CValue {
	
	protected:
		
		void updateHashArgs (SHA1& hash) {
			for (map<string,shared_ptr<CValueRef>>::iterator it = values.begin(); it != values.end(); it++) {
				hash.update ("key:");
				hash.update (it -> first);
				hash.update ("ref:");
				it -> second -> updateHash (hash);
			}
		}
	
	private:
		
		map<string,shared_ptr<CValueRef>> values;
		
	public:
		
		int asInt () {
			return 0;
		}
		
		string asString () {
			stringstream ss;
			ss << "{";
			bool first = true;
			
			for (map<string,shared_ptr<CValueRef>>::iterator it = values.begin(); it != values.end(); it++) {
				if (!first)
					ss << ", ";
				else
					first = false;
				
				ss << "\"" << it -> first << "\": \"" << it -> second -> getValue () -> asString () << "\"";
				
			}
			ss << "}";
			return ss.str();
		}

		ValueType getType () {
			return ValueDict;
		}
		
		void append (const string& index, shared_ptr<CValueRef> value) {
			values.insert (pair<string, shared_ptr<CValueRef>> (index, value));
		}
		
		shared_ptr<CValue> subscript (const string& index) {
			
			map<string,shared_ptr<CValueRef>>::iterator it = values.find (index);
			if (it == values.end ()) {
				return shared_ptr<CValue> (new CVoidValue());
			}
			
			shared_ptr<CValueRef> found = it -> second;
			return found -> getValue ();
		}
		
		shared_ptr<CValueRef> subscriptRef (const string& index) {

			map<string,shared_ptr<CValueRef>>::iterator it = values.find (index);
			if (it == values.end ()) {
				shared_ptr<CValue> newValue = shared_ptr<CValue> (new CVoidValue ());
				shared_ptr<CValueRef> newRef = shared_ptr<CValueRef> (new CValueRef (newValue));
				values.insert (pair<string, shared_ptr<CValueRef>> (index, newRef));
				return newRef;
			}
			
			return it -> second;
			
		}
		
		int getLength () {
			return values.size ();
		}
		
		void getKeys (list<string>& keys) {
			for (map<string,shared_ptr<CValueRef>>::iterator it = values.begin(); it != values.end(); it++) {
				keys.push_back (it -> first);
			}
		}
		
	
};


class CIntValue: public CValue {
	
	protected:
		
		void updateHashArgs (SHA1& hash) {
			stringstream ss;
			ss << intValue;
			hash.update (ss.str());
		}
	
	private:
		
		int intValue;
		
	public:
		
		CIntValue (int p_intValue) {
			intValue = p_intValue;
		}
		
		int asInt () {
			return intValue;
		}
		
		string asString () {
			char buf[16];
			snprintf (buf, 16, "%d", intValue);
			return string (buf);
		}
		
		ValueType getType () {
			return ValueInt;
		}
	
};

class CStringValue: public CValue {

	protected:
		
		void updateHashArgs (SHA1& hash) {
			hash.update (stringValue);
		}
		
	private:
		
		string stringValue;
		
	public:
		
		CStringValue (const string& p_stringValue) {
			stringValue = p_stringValue;
		}
		
		int asInt () {
			return strtol (stringValue.c_str(), NULL, 0);
		}
		
		string asString () {
			return stringValue;
		}
		
		ValueType getType () {
			return ValueString;
		}
		
		int getLength () {
			return stringValue.size ();
		}
	
};

#endif /* __VALUE_H__ */
