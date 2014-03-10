#define _BSD_SOURCE
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdexcept>
#include <iostream>
#include <cstring>

#ifdef _MSC_VER
# include <windows.h>
# include <direct.h>
# include <lmcons.h>
#else
# include <dirent.h>
# include <sys/wait.h>
# include <unistd.h>
# include <sys/select.h>
# include <sys/sendfile.h>
# include <fcntl.h>
#endif

#include "sys_funcs.h"

string getPathSeparator () {
#ifdef _MSC_VER
	return "\\";
#else
	return "/";
#endif
}

string getAnyPathSeparator () {
	return "\\/";
}

bool isDirectory (const string& path) {

#ifdef _MSC_VER

	DWORD attrs = GetFileAttributes (makeSysSeparators(path).c_str());
	if (attrs == INVALID_FILE_ATTRIBUTES)
		return false;
		
	return ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0);
	
#else

	struct stat s;
	
	if (lstat (makeSysSeparators(path).c_str(), &s) != 0)
		return false;
	
	return (S_ISDIR (s.st_mode));
	
#endif		
	
}

bool fileExists (const string& path) {

#ifdef _MSC_VER

	DWORD attrs = GetFileAttributes (makeSysSeparators(path).c_str());
	if (attrs == INVALID_FILE_ATTRIBUTES)
		return false;
		
	return true;
	
#else

	struct stat s;
	
	if (lstat (makeSysSeparators(path).c_str(), &s) != 0)
		return false;
	
	return true;
	
#endif		
	
}

string extractFileName (const string& fname) {

	size_t pos = fname.find_last_of (getAnyPathSeparator ());
	
	if (pos == string::npos)
		return fname;
	else 
		return fname.substr (pos+1);

}

bool isAbsolutePath (const string& path) {

	if (path.empty ())
		return false;
	
	char firstChar = path.at (0);
	if (getAnyPathSeparator().find (firstChar) != string::npos)
		return true;
	
#ifdef _MSC_VER
		
	if (path.length () >= 3) {
		if (isalpha (path.at (0)) && (path.at (1) == ':') && (getAnyPathSeparator().find (path.at(2)) != string::npos))
			return true;
	}
		
#endif
		
	return false;
	
}

void getFilesInPath (list<string>& result, const string& relpath, const string& path) {

#ifdef _MSC_VER

	HANDLE hFind;
	WIN32_FIND_DATA data;
	string searchPath = path + "\\*";

	hFind = FindFirstFile (searchPath.c_str(), &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			string fname (data.cFileName);
			
			if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
				if (fname != "." && fname != "..") {
					string newpath = path + getPathSeparator() + fname;
					string newrelpath = (relpath.empty()) ? fname : (relpath + "/" + fname);
					getFilesInPath (result, newrelpath, newpath);
				}
			} else
				result.push_back (relpath.empty() ? fname : (relpath + "/" + fname));
				
		} while (FindNextFile(hFind, &data));
		
		FindClose (hFind);
	} else {
		printf ("hFind %u path %s\n", hFind, path.c_str());
	}

#else

	DIR *dp;
	dirent *d;

	dp = opendir (path.c_str());
	if (dp == NULL)
		return;
	
	while((d = readdir(dp)) != NULL) {
		
		string fname (d -> d_name);
		
		if (d -> d_type == DT_DIR) {
			if (fname != "." && fname != "..") {
				string newpath = path + getPathSeparator() + fname;
				string newrelpath = (relpath.empty()) ? fname : (relpath + "/" + fname);
				getFilesInPath (result, newrelpath, newpath);
			}
		} else
			result.push_back (relpath.empty() ? fname : (relpath + "/" + fname));
	}
	
	closedir (dp);
	
#endif

}

void copyOneFile (const string& path, const string& to) {

#ifdef _MSC_VER

	if (!CopyFile (path.c_str(), to.c_str(), FALSE))
		throw runtime_error ("Failed to copy " + path + " to " + to);

#else
		
    int input, output;    

	if ((input = open (path.c_str(), O_RDONLY)) == -1)
		throw runtime_error ("Failed to copy " + path + " to " + to);
	
    struct stat fileinfo = {0};
    fstat(input, &fileinfo);
	
    if ((output = open(to.c_str(), O_RDWR | O_CREAT | O_TRUNC, fileinfo.st_mode)) == -1)
		throw runtime_error ("Failed to copy " + path + " to " + to);

    off_t bytesCopied = 0;
    int result = sendfile (output, input, &bytesCopied, fileinfo.st_size);
	if (result == -1)
		throw runtime_error ("Failed to copy " + path + " to " + to);
	
    close(input);
    close(output);
	
#endif

}


void copyFile (const string& path, const string& to) {
	
	if (isDirectory (path)) {
	
		if (fileExists (to) && !isDirectory (to)) 
			throw runtime_error ("Cannot copy directory " + path + " to regular file " + to);
		
		string destDir = to;
		if (isDirectory (destDir)) 
			destDir = destDir + getPathSeparator() + extractFileName (path);
		
		makeDirs (destDir);
		
#ifdef _MSC_VER
		
		HANDLE hFind;
		WIN32_FIND_DATA data;
		string searchPath = path + "\\*";

		hFind = FindFirstFile (searchPath.c_str(), &data);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				string fname (data.cFileName);
				
				if (fname != "." && fname != "..") 
					copyFile (path + getPathSeparator() + fname, destDir);
					
			} while (FindNextFile(hFind, &data));
			
			FindClose (hFind);
		} else 
			throw runtime_error ("Failed to scan: " + path);
		
#else
			
		DIR *dp;
		dirent *d;

		dp = opendir (path.c_str());
		if (dp == NULL)
			throw runtime_error ("Failed to opendir(): " + path);

		while((d = readdir(dp)) != NULL) {
			
			string fname (d -> d_name);
			
			if (fname != "." && fname != "..") 
				copyFile (path + getPathSeparator() + fname, destDir);
				
		}

		closedir (dp);
		
#endif		
		
		
	} else {
		
		string copyTo = to;
		
		if (isDirectory (copyTo))
			copyTo = copyTo + getPathSeparator() + extractFileName (path);
		
		copyOneFile (path, copyTo);
	}

}


void deleteFileOrDir (const string& path) {

#ifdef _MSC_VER

	DWORD attrs = GetFileAttributes (path.c_str());
	if (attrs == INVALID_FILE_ATTRIBUTES)
		return;
		
	if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
	
		HANDLE hFind;
		WIN32_FIND_DATA data;
		string searchPath = path + "\\*";

		hFind = FindFirstFile (searchPath.c_str(), &data);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				string fname (data.cFileName);
				
				if (fname != "." && fname != "..") 
					deleteFileOrDir (path + getPathSeparator() + fname);
					
			} while (FindNextFile(hFind, &data));
			
			FindClose (hFind);
		} else 
			throw runtime_error ("Failed to clean: " + path);
			
		if (!RemoveDirectory (path.c_str()))
			throw runtime_error ("Failed to delete: " + path);
	
	} else {
		if (!DeleteFile (path.c_str()))
			throw runtime_error ("Failed to delete: " + path);
	}

#else

	struct stat s;
	string spath = makeSysSeparators (path);
	
	if (lstat (spath.c_str(), &s) != 0)
		return;
	
	if (S_ISDIR (s.st_mode)) {
		
		DIR *dp;
		dirent *d;

		dp = opendir (spath.c_str());
		if (dp == NULL)
			throw runtime_error ("Failed to opendir(): " + spath);

		while((d = readdir(dp)) != NULL) {
			
			string fname (d -> d_name);
			
			if (fname != "." && fname != "..") 
				deleteFileOrDir (spath + getPathSeparator() + fname);
				
		}

		closedir (dp);
		rmdir (spath.c_str());
		
	} else {
		if (unlink (spath.c_str()))
			throw runtime_error ("Failed to delete: " + spath);
	}


#endif	

}


int runCommand (const list<string>& params, string *capture_stdout, map<string,string>* env) {

#ifdef _MSC_VER

	string cmdline;
	
	for (list<string>::const_iterator it = params.begin (); it != params.end (); it++) {
		if (!cmdline.empty ())
			cmdline += " ";
			
		string param = (*it);
		if (param.find (' ') == string::npos)
			cmdline += param;
		else {
			cmdline += "\"" + param + "\"";
		}
	}
	
	int envBlockSize = 0;
	char *envBlock = NULL;
	
	if (env != NULL) {
		for (map<string,string>::iterator it = env -> begin (); it != env -> end (); it ++) 
			envBlockSize += it -> first.length () + 1 + it -> second.length() + 1;
		
		envBlockSize ++;
		
		envBlock = (char *) malloc (envBlockSize);
		memset (envBlock, 0, envBlockSize);
		char *curPos = envBlock;
		
		for (map<string,string>::iterator it = env -> begin (); it != env -> end (); it ++)  {
			string s = it -> first + "=" + it -> second;
			strcpy (curPos, s.c_str());
			curPos += s.length() + 1;
			
			if (equalsIgnoreCase (it -> first, "path")) {
				_putenv (s.c_str());
			}
			
		}
	}
	
	HANDLE hStdoutRead, hStdoutWrite;
	
	if (capture_stdout) {
	
		SECURITY_ATTRIBUTES sa;
		
		memset (&sa, 0, sizeof (SECURITY_ATTRIBUTES));
		sa.nLength = sizeof (SECURITY_ATTRIBUTES);
		sa.bInheritHandle = TRUE;
		
		CreatePipe (&hStdoutRead, &hStdoutWrite, &sa, 0);
		
		SetHandleInformation (hStdoutRead, HANDLE_FLAG_INHERIT, 0);
		
	}
	
	STARTUPINFO startupInfo;
	PROCESS_INFORMATION procInfo;
	
	memset (&startupInfo, 0, sizeof (STARTUPINFO));
	startupInfo.cb = sizeof (STARTUPINFO);
	startupInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE); 
	startupInfo.hStdOutput = capture_stdout ? hStdoutWrite : GetStdHandle (STD_OUTPUT_HANDLE); 
	startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE); 
	startupInfo.dwFlags |= STARTF_USESTDHANDLES;	
	
	memset (&procInfo, 0, sizeof (PROCESS_INFORMATION));
	
	cout << "run: \"" << cmdline << "\"" << endl;
	
	char *cmdlineBuf = new char[65536];
	strncpy (cmdlineBuf, cmdline.c_str(), 65536);

	BOOL rc = CreateProcess (NULL, cmdlineBuf, NULL, NULL, TRUE, 0, envBlock, NULL, &startupInfo, &procInfo);
	
	delete cmdlineBuf;
	
	if (!rc) 
		throw runtime_error ("CreateProcess() failed.");
		
	if (capture_stdout) {
	
		CloseHandle (hStdoutWrite);

		while (true) {

			char buf[1024+1];
			DWORD gotSize = 0;
			
			int rc = ReadFile (hStdoutRead, buf, 1024, &gotSize, 0);
			if (rc == 0)
				break;

			buf[gotSize] = 0;
			(*capture_stdout) += string (buf);
			
		}	
		
		CloseHandle (hStdoutRead);
		
	}
		
	WaitForSingleObject (procInfo.hProcess, INFINITE);
	
	DWORD exitCode = 0xFFFFFFFF;
	
	GetExitCodeProcess (procInfo.hProcess, &exitCode);
	
	CloseHandle (procInfo.hThread);
	CloseHandle (procInfo.hProcess);
	
	if (envBlock != NULL)
		free (envBlock);
	
	cout << "exit code: " << exitCode << endl;
	
	return (int) exitCode;
	
#else
	
	char **envp = NULL;
	static char *oldpath = NULL;
	
	if (env != NULL) {
		envp = new char* [env -> size () + 1];
		int i = 0;
		
		for (map<string,string>::iterator it = env -> begin (); it != env -> end (); it ++)  {
			string s = it -> first + "=" + it -> second;
			envp [i] = strdup (s.c_str());
			
			if (equalsIgnoreCase (it -> first, "path")) {
				char *newpath = strdup (s.c_str());
				putenv (newpath);
				if (oldpath != NULL) 
					free (oldpath);
				oldpath = newpath;
			}
			
			i++;
		}
		
		envp [i++] = NULL;
	}
	
	
	char **args = new char* [params.size () + 1];
	int rc = -1;
	int i = 0;
	
	cout << "run:";
	
	for (list<string>::const_iterator it = params.begin (); it != params.end (); it++) {
		cout << " " << (*it);
		args[i] = strdup (string (*it).c_str());
		i ++;
	}
	
	args[params.size()] = NULL;
	
	cout << endl;
	
	int pipe_fds[2];
	
	if (capture_stdout != NULL)
		pipe (pipe_fds);
	
	pid_t child_pid = fork();
	
	if (child_pid == 0) { // child
		
		if (stdout != NULL) {
			dup2 (pipe_fds[1], STDOUT_FILENO);
			close (pipe_fds[0]);
		}
		
		if (envp != NULL)
			execvpe (args[0], args, envp);
		else
			execvp (args[0], args);
		
		exit (-1);
		
    } else if (child_pid < 0) { // failed fork ()
	
		return -1;
		
	} else { // parent
		
		if (capture_stdout != NULL) {
		
			close (pipe_fds[1]);
			
			fd_set rfds, efds;
			
			while (true) {
				
				char buf[1024];
				
				FD_ZERO (&rfds);
				FD_ZERO (&efds);
				FD_SET (pipe_fds [0], &rfds); 				
				FD_SET (pipe_fds [0], &efds); 				
				
				select (pipe_fds[0] + 1, &rfds, NULL, &efds, NULL); 
				if (FD_ISSET (pipe_fds[0], &rfds)) {
					
					int rc = read (pipe_fds[0], buf, 1023);
					if (rc > 0) {
						buf[rc] = 0;
						(*capture_stdout) += string (buf);
					} else
						break;
				}
				
				if (FD_ISSET (pipe_fds[0], &efds)) 
					break;
				
			}
		} 
			
		waitpid (child_pid, &rc, 0);
		
	}
	
	if (oldpath != NULL) {
		putenv (oldpath);
	}
	
	if (envp != NULL) {
		for (i = 0; envp[i]; i++)
			free (envp[i]);
		delete envp;
	}
	
	for (size_t i = 0; i < params.size(); i++) 
		free (args[i]);
	
	delete args;
	
	return rc;

#endif

}

string getCurrentDirectory () {
	
	char cwd[FILENAME_MAX];
	
#ifdef _MSC_VER
	_getcwd (cwd, FILENAME_MAX);
#else
	getcwd (cwd, FILENAME_MAX);
#endif
	
	return string (cwd);
	
}

void setCurrentDirectory (const string& dirPath) {
	
	int rc;
	
#ifdef _MSC_VER
	rc = _chdir (makeSysSeparators (dirPath).c_str());
#else
	rc = chdir (makeSysSeparators (dirPath).c_str());
#endif

	if (rc != 0)
		throw runtime_error ("Failed to cd(" + dirPath + ")");
	
}

void makeDirs (const string& dirPath) {

	size_t pos = 0;
	string thisPath = "";
	
	while (pos < dirPath.length()) {
		size_t nextSep = dirPath.find_first_of (getPathSeparator (), pos);
		if (nextSep == string::npos)
			nextSep = dirPath.length ();
		else
			nextSep ++;
		
		string pathComp = dirPath.substr (pos, nextSep - pos);
		
		// cout << "pathComp: " << pathComp << " .. ";
		
		if (!pathComp.empty ()) {
			thisPath += pathComp;
			string makePath = thisPath;
			if (makePath.substr (makePath.length()-1, 1) == getPathSeparator ()) {
				if (makePath.find_first_of (getPathSeparator()) != makePath.length()-1) 
					makePath.resize (makePath.length()-1);
			}
			// cout << "making path: " << makePath << endl;

#ifdef _MSC_VER
			_mkdir (makePath.c_str());
#else
			mkdir (makePath.c_str(), 0775);
#endif			
		}
		
		pos = nextSep;
		
	}
	
}

string makeSysSeparators (const string& path) {
	string result = path;
	size_t pos = 0;
	
	while ((pos = result.find_first_of (getAnyPathSeparator (), pos)) != string::npos) {
		result[pos] = getPathSeparator().at (0);
		pos ++;
	}
	
	return result; 
}

list<string> getFilesInPath (const string& path) {

	list<string> result;
	string relative_to = path;
	
	if (relative_to != ".") {
		if (!relative_to.empty ()) {
			char last = relative_to.back ();
			if (getAnyPathSeparator().find (last) != string::npos)
				relative_to += "/";
		}
	} else
		relative_to = "";
	
	getFilesInPath (result, relative_to, makeSysSeparators (path));
	
	return result;
	
}

string getAbsolutePath (const string& relPath) {

#ifdef _MSC_VER

	char buf[MAX_PATH];
	
	if (GetFullPathName (makeSysSeparators (relPath).c_str(), MAX_PATH, buf, NULL) == 0)
		throw runtime_error ("Failed to get absolute path name for '" + relPath + "'");

	return string (buf);

#else

	char buf[FILENAME_MAX];
	realpath (makeSysSeparators(relPath).c_str(), buf);
	return string (buf);
	
#endif
	
}

#ifdef _MSC_VER

time_t time_t_from_ft (FILETIME *ft) {

	ULARGE_INTEGER ull;
	
	ull.LowPart = ft -> dwLowDateTime;
	ull.HighPart = ft -> dwHighDateTime;
	double d = ull.QuadPart;
	double td = d / 10000000.0 - 11644473600.0;
	
	return (time_t) td;
}

#endif

bool getFileInfo (const string& fileName, long* size, time_t* mtime) {

#ifdef _MSC_VER

	HANDLE hFile = CreateFile (makeSysSeparators (fileName).c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
		
	BY_HANDLE_FILE_INFORMATION fileInfo;
	if (GetFileInformationByHandle (hFile, &fileInfo) == 0) {
		CloseHandle (hFile);
		return false;
	}
	
	if (size != NULL)
		(*size) = fileInfo.nFileSizeLow;
	if (mtime != NULL)
		(*mtime) = time_t_from_ft (&fileInfo.ftLastWriteTime);
		
	CloseHandle (hFile);
	return true;

#else

	struct stat s;
	
	if (lstat (makeSysSeparators (fileName).c_str(), &s) != 0)
		return false;

	if (size != NULL)
		(*size) = s.st_size;
	if (mtime != NULL)
		(*mtime) = s.st_mtime;
	
	return true;
	
#endif
	
}

#ifdef _MSC_VER

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;

bool getWindowsBit (bool & isWindows64bit)
{

#if _WIN64

    isWindows64bit =  true;
    return true;

#elif _WIN32

    BOOL isWow64 = FALSE;

    //IsWow64Process is not available on all supported versions of Windows.
    //Use GetModuleHandle to get a handle to the DLL that contains the function
    //and GetProcAddress to get a pointer to the function if available.

    LPFN_ISWOW64PROCESS fnIsWow64Process  = (LPFN_ISWOW64PROCESS)  GetProcAddress(GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

    if(fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(), &isWow64))
            return false;

        if(isWow64)
            isWindows64bit =  true;
        else
            isWindows64bit =  false;

        return true;
    }
    else
        return false;

#else

    return false;

#endif
}

#endif

int getBitness () {
#ifdef _MSC_VER

	bool isWin64 = false;
	if (!getWindowsBit (isWin64))
		throw runtime_error ("Cannot detect Windows bitness");

	return isWin64 ? 64 : 32;

#else

#ifdef __LP64__	
	return 64;
#else
	return 32;
#endif
	
#endif
}

string getComputerName () {
#ifdef _MSC_VER

	char buf [MAX_COMPUTERNAME_LENGTH + 1];
	DWORD bufSize = MAX_COMPUTERNAME_LENGTH + 1;
	
	if (GetComputerName (buf, &bufSize) == 0)
		throw runtime_error ("GetComputerName() failed");
	
	return string (buf);

#else
	
	char buf[HOST_NAME_MAX];
	if (gethostname (buf, HOST_NAME_MAX))
		throw runtime_error ("gethostname() failed");
	
	return string (buf);
	
#endif
}

string getUserName () {
#ifdef _MSC_VER

	char buf [UNLEN + 1];
	DWORD bufSize = UNLEN + 1;
	
	if (GetUserName (buf, &bufSize) == 0)
		throw runtime_error ("GetUserName() failed");
	
	return string (buf);

#else

	return string (getlogin ());
	
#endif
}

string getPlatform () {
#ifdef _MSC_VER	
	string platform = "windows";
#else	
	string platform = "unix";
#endif	
	return platform;
}

void getEnvironment (map<string,string>& environment) {
#ifdef _MSC_VER

	char *eenv = GetEnvironmentStrings ();
	char *p = eenv;
	
	while (*p) {
	
		int len = strlen (p);
		string s (p, len);
		
		string key, value;
		size_t pos = s.find ("=");
		if (pos != string::npos) {
			key = s.substr (0, pos);
			value = s.substr (pos+1);
		} else {
			key = s;
			value = "";
		}
		
		environment[key] = value;
		p += len + 1;
	}
	
	FreeEnvironmentStrings (eenv);
	

#else
	
	char **p = environ;
	
	while (*p) {

		int len = strlen (*p);
		string s (*p, len);
		
		string key, value;
		size_t pos = s.find ("=");
		if (pos != string::npos) {
			key = s.substr (0, pos);
			value = s.substr (pos+1);
		} else {
			key = s;
			value = "";
		}
		
		environment[key] = value;
		p ++;
		
	}

#endif
}

#ifdef _MSC_VER

inline int c99_vsnprintf(char* str, size_t size, const char* format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}


int snprintf (char* str, size_t size, const char* format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(str, size, format, ap);
    va_end(ap);

    return count;
}

#endif // _MSC_VER

bool equalsIgnoreCase (const string& str1, const string& str2) {
    if (str1.size() != str2.size()) {
        return false;
    }
    for (string::const_iterator c1 = str1.begin(), c2 = str2.begin(); c1 != str1.end(); ++c1, ++c2) {
        if (tolower(*c1) != tolower(*c2)) {
            return false;
        }
    }
    return true;
}

