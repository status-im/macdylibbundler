#pragma once

#ifndef DYLIBBUNDLER_UTILS_H
#define DYLIBBUNDLER_UTILS_H

#include <map>
#include <string>
#include <vector>
#include "DylibBundler.h"

std::string filePrefix(const std::string& in);
std::string stripPrefix(const std::string& in);
std::string stripLSlash(const std::string& in);

std::string getFrameworkRoot(const std::string& in);
std::string getFrameworkPath(const std::string& in);

// trim from end (in place)
void rtrim_in_place(std::string& s);
// trim from end (copying)
std::string rtrim(std::string s);
void tokenize(const std::string& str, const char* delimiters, std::vector<std::string>* tokens);

std::vector<std::string> lsDir(const std::string& path);
bool fileExists(const std::string& filename);
bool isRpath(const std::string& path);
std::string bundleExecutableName(const std::string& app_bundle_path);

// execute a command in the native shell and return output in string
std::string systemOutput(const std::string& cmd);

void otool(const std::string& flags, const std::string& file, std::vector<std::string>& lines);
void parseLoadCommands(const std::string& file, const std::string& cmd, const std::string& value, std::vector<std::string>& lines);
void parseLoadCommands(const std::string& file, const std::map<std::string,std::string>& cmds_values, std::map<std::string,std::vector<std::string>>& cmds_results);

void createQtConf(std::string directory);

#endif
