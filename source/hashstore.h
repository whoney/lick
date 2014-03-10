#ifndef __HASHSTORE_H__
#define __HASHSTORE_H__

#include <string>
#include <map>
#include <set>

using namespace std;

class CTargetHashStore {

	private:
	
		set<string> prevRunHashes;
		set<string> hashes;
		bool touched;
		
	public:
	
		CTargetHashStore () {
			touched = false;
		}
	
		bool containsHash (const string& hash);
		void addHash (const string& hash);
		void clear ();
		
		void load (const string& hash);
		set<string>& getHashes ();

};

class CModuleHashStore {

	private:
	
		map<string,CTargetHashStore> targetHashes;
		
	public:
	
		bool containsHash (const string& target, const string& hash);
		void addHash (const string& target, const string& hash);
		void clear ();
		
		void load (const string& target, const string& hash);
		map<string,CTargetHashStore>& getHashes ();

};

class CHashStore {
	
	private:
	
		map<string,CModuleHashStore> moduleHashes;
	
		string storeFileName;
		bool storeOpened;
		
		void openStore ();
		void saveStore ();
		
	public:
		
		CHashStore ();
		
		bool containsHash (const string& module, const string& target, const string& hash);
		void addHash (const string& module, const string& target, const string& hash);
		void clear (const string& module);
		
		void setNameFromModule (const string& moduleName);
	
};

extern CHashStore hashStore;

#endif /* __HASHSTORE_H__ */
