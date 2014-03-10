#ifndef __SYS_FUNCS_H__
#define __SYS_FUNCS_H__

#include <sys/types.h>
#include <list>
#include <string>
#include <map>

using namespace std;

string getPathSeparator ();
string getAnyPathSeparator ();
string makeSysSeparators (const string& path);
list<string> getFilesInPath (const string& path);
int runCommand (const list<string>& params, string *capture_stdout, map<string,string>* env);
string getCurrentDirectory ();
void setCurrentDirectory (const string& dirPath);
void makeDirs (const string& dirPath);
string getAbsolutePath (const string& relPath);
bool getFileInfo (const string& fileName, long* size, time_t* mtime);
void deleteFileOrDir (const string& path);
void copyFile (const string& path, const string& to);
string getComputerName ();
string getUserName ();
string getPlatform ();
int getBitness ();
void getEnvironment (map<string,string>& environment);
string extractFileName (const string& fname);
bool isDirectory (const string& path);

#ifdef _MSC_VER
#define ENV_PATH_SEPARATOR (';')
#else
#define ENV_PATH_SEPARATOR (':')
#endif

#ifdef _MSC_VER
int snprintf (char* str, size_t size, const char* format, ...);
#endif

bool equalsIgnoreCase (const string& str1, const string& str2);

#endif /* __SYS_FUNCS_H__ */

