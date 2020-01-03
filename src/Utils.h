#ifndef _utils_h_
#define _utils_h_

#include <string>
#include <vector>

// class Library;

std::string filePrefix(std::string in);
std::string stripPrefix(std::string in);

std::string getFrameworkRoot(std::string in);
std::string getFrameworkPath(std::string in);

std::string stripLSlash(std::string in);

// trim from end (in place)
void rtrim_in_place(std::string& s);
// trim from end (copying)
std::string rtrim(std::string s);

// executes a command in the native shell and returns output in string
std::string systemOutput(const std::string& cmd);
// like 'system', runs a command on the system shell, but also prints the command to stdout.
int systemp(const std::string& cmd);

void tokenize(const std::string& str, const char* delimiters, std::vector<std::string>*);

std::vector<std::string> lsDir(std::string path);
bool fileExists(std::string filename);
bool isRpath(const std::string& path);

std::string bundleExecutableName(const std::string& app_bundle_path);

void changeId(std::string binary_file, std::string new_id);
void changeInstallName(std::string binary_file, std::string old_name, std::string new_name);

void copyFile(std::string from, std::string to);
void deleteFile(std::string path, bool overwrite);
void deleteFile(std::string path);

bool mkdir(std::string path);
void createDestDir();
std::string getUserInputDirForFile(const std::string& filename);

// check the same paths the system would search for dylibs
void initSearchPaths();

#endif
