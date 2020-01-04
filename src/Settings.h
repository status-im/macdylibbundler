#pragma once

#ifndef _settings_
#define _settings_

#include <string>
#include <vector>

#ifndef __clang__
#include <sys/types.h>
#endif

namespace Settings {

bool isPrefixBundled(std::string prefix);
bool isPrefixIgnored(std::string prefix);
void ignorePrefix(std::string prefix);

bool appBundleProvided();
std::string appBundle();
void appBundle(std::string path);

std::string destFolder();
void destFolder(std::string path);

std::string executableFolder();
std::string frameworksFolder();
std::string pluginsFolder();
std::string resourcesFolder();

void addFileToFix(std::string path);
std::string fileToFix(int n);
std::vector<std::string> filesToFix();
size_t filesToFixCount();

std::string insideLibPath();
void insideLibPath(std::string p);

void addSearchPath(std::string path);
std::string searchPath(int n);
size_t searchPathCount();

void addUserSearchPath(std::string path);
std::string userSearchPath(int n);
size_t userSearchPathCount();

bool canCreateDir();
void canCreateDir(bool permission);

bool canOverwriteDir();
void canOverwriteDir(bool permission);

bool canOverwriteFiles();
void canOverwriteFiles(bool permission);

bool bundleLibs();
void bundleLibs(bool status);

bool bundleFrameworks();
void bundleFrameworks(bool status);

bool quietOutput();
void quietOutput(bool status);

bool verboseOutput();
void verboseOutput(bool status);

bool missingPrefixes();
void missingPrefixes(bool status);

std::string getFullPath(const std::string& rpath);
void rpathToFullPath(const std::string& rpath, const std::string& fullpath);
bool rpathFound(const std::string& rpath);

std::vector<std::string> getRpathsForFile(const std::string& file);
void addRpathForFile(const std::string& file, const std::string& rpath);
bool fileHasRpath(const std::string& file);

} // namespace Settings

#endif

std::string stripLSlash(const std::string& in);
