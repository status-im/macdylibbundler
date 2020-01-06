#pragma once

#ifndef DYLIBBUNDLER_UTILS_H
#define DYLIBBUNDLER_UTILS_H

#include <map>
#include <string>
#include <vector>

std::string filePrefix(const std::string& in);
std::string stripPrefix(const std::string& in);

std::string getFrameworkRoot(const std::string& in);
std::string getFrameworkPath(const std::string& in);

std::string stripLSlash(const std::string& in);

// trim from end (in place)
void rtrim_in_place(std::string& s);
// trim from end (copying)
std::string rtrim(std::string s);

// execute a command in the native shell and return output in string
std::string systemOutput(const std::string& cmd);
// run a command in the system shell (like 'system') but also print the command to stdout
int systemp(const std::string& cmd);

void tokenize(const std::string& str, const char* delimiters, std::vector<std::string>*);

std::vector<std::string> lsDir(const std::string& path);
bool fileExists(const std::string& filename);
bool isRpath(const std::string& path);

std::string bundleExecutableName(const std::string& app_bundle_path);

void changeId(const std::string& binary_file, const std::string& new_id);
void changeInstallName(const std::string& binary_file, const std::string& old_name, const std::string& new_name);

void copyFile(const std::string& from, const std::string& to);
void deleteFile(const std::string& path, bool overwrite);
void deleteFile(const std::string& path);
bool mkdir(const std::string& path);

void createDestDir();

std::string getUserInputDirForFile(const std::string& filename, const std::string& dependent_file);

void otool(const std::string& flags, const std::string& file, std::vector<std::string>& lines);
void parseLoadCommands(const std::string& file, const std::string& cmd, const std::string& value, std::vector<std::string>& lines);
void parseLoadCommands(const std::string& file, const std::map<std::string,std::string>& cmds_values, std::map<std::string,std::vector<std::string>>& cmds_results);

std::string searchFilenameInRpaths(const std::string& rpath_file, const std::string& dependent_file);
std::string searchFilenameInRpaths(const std::string& rpath_file);

// check the same paths the system would search for dylibs
void initSearchPaths();

void createQtConf(std::string directory);

#endif
