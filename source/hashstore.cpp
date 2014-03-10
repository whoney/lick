#include <iostream>
#include <fstream>

#include "hashstore.h"
#include "sys_funcs.h"

CHashStore hashStore;

bool CTargetHashStore::containsHash (const string& hash) {

	touched = true;

	if (prevRunHashes.find (hash) != prevRunHashes.end ()) {
		hashes.insert (hash);
		return true;
	}
	
	if (hashes.find (hash) != hashes.end ())
		return true;
		
	return false;
	
}

void CTargetHashStore::addHash (const string& hash) {
	touched = true;
	hashes.insert (hash);
}

void CTargetHashStore::clear () {
	touched = true;
	hashes.clear ();
}

void CTargetHashStore::load (const string& hash) {
	prevRunHashes.insert (hash);
}

set<string>& CTargetHashStore::getHashes () {
	if (touched)
		return hashes;
	else
		return prevRunHashes;
}

bool CModuleHashStore::containsHash (const string& target, const string& hash) {

	map<string,CTargetHashStore>::iterator it = targetHashes.find (target);
	if (it == targetHashes.end ())
		return false;
		
	return it -> second.containsHash (hash);

}

void CModuleHashStore::addHash (const string& target, const string& hash) {
	targetHashes[target].addHash (hash);
}

void CModuleHashStore::clear () {

	for (map<string,CTargetHashStore>::iterator it = targetHashes.begin (); it != targetHashes.end (); it++) 
		it -> second.clear ();

}

void CModuleHashStore::load (const string& target, const string& hash) {
	targetHashes[target].load (hash);
}

map<string,CTargetHashStore>& CModuleHashStore::getHashes () {
	return targetHashes;
}

CHashStore::CHashStore () {
	
	storeOpened = false;
	storeFileName = getCurrentDirectory () + getPathSeparator () + ".lick" + getPathSeparator () + "hashstore";
	
}

void CHashStore::setNameFromModule (const string& moduleName) {

	if (!storeOpened) {
	
		size_t pos = moduleName.find_last_of (getAnyPathSeparator ());
		string dirName = moduleName.substr (0, pos);
		
		storeFileName = dirName + getPathSeparator () + ".lick" + getPathSeparator () + "hashstore";
		
	}
	
}

void CHashStore::openStore () {
	if (!storeOpened) {

		size_t lastPos = storeFileName.find_last_of (getPathSeparator ());
		if (lastPos != string::npos) {
			string dirPath = storeFileName.substr (0, lastPos);
			if (!dirPath.empty ()) {
				makeDirs (dirPath);
			}
			
			ifstream ifs;
			ifs.open (storeFileName);
			if (ifs.is_open ()) {

				string line;
				
				while (getline (ifs, line)) {
					if (!line.empty ()) {
					
						size_t pos = line.find_first_of ("|");
						if (pos == string::npos)
							continue;
							
						string moduleName = line.substr (0, pos);
						
						size_t pos1 = line.find_first_of ("|", pos+1);
						if (pos1 == string::npos)
							continue;
							
						string targetName = line.substr (pos+1, pos1 - (pos + 1));
						string hash = line.substr (pos1+1);
						
						moduleHashes[moduleName].load (targetName, hash);
						
					}
				}
			
				ifs.close ();
			}
		}
		
		storeOpened = true;
	}
}

void CHashStore::saveStore () {

	if (storeOpened) {
		
		ofstream ofs;
		ofs.open (storeFileName);
		if (ofs.is_open ()) {
			
			for (map<string,CModuleHashStore>::iterator it = moduleHashes.begin (); it != moduleHashes.end (); it++) {
			
				string moduleName = it -> first;
				map<string,CTargetHashStore>& targetHashes = it -> second.getHashes ();
			
				for (map<string,CTargetHashStore>::iterator i1 = targetHashes.begin (); i1 != targetHashes.end (); i1++) {
				
					string targetName = i1 -> first;
					set<string>& hashes = i1 -> second.getHashes ();
					
					for (set<string>::iterator i2 = hashes.begin (); i2 != hashes.end (); i2++) {
					
						string hash = (*i2);
						ofs << moduleName << "|" << targetName << "|" << hash << endl;
					
					}
				
				}
			
			}
			
			ofs.close ();
		}
		
	}
		
}

bool CHashStore::containsHash (const string& module, const string& target, const string& hash) {

	openStore ();
	map<string,CModuleHashStore>::iterator it = moduleHashes.find (module);
	if (it == moduleHashes.end())
		return false;
		
	bool rc = it -> second.containsHash (target, hash);
	saveStore ();
	return rc;
}

void CHashStore::addHash (const string& module, const string& target, const string& hash) {

	openStore ();
	moduleHashes[module].addHash (target, hash);
	saveStore ();

}

void CHashStore::clear (const string& module) {

	openStore ();

	map<string,CModuleHashStore>::iterator it = moduleHashes.find (module);
	if (it != moduleHashes.end())
		it -> second.clear ();

	saveStore ();

}

