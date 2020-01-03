#ifndef _settings_
#define _settings_

#include <string>
#include <vector>

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
size_t searchPathCount();
std::string searchPath(int n);

void addUserSearchPath(std::string path);
size_t userSearchPathCount();
std::string userSearchPath(int n);

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

} // namespace Settings

#endif
