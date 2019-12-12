#ifndef _settings_
#define _settings_

#include <string>

namespace Settings {

bool isPrefixBundled(std::string prefix);
bool isPrefixIgnored(std::string prefix);
void ignore_prefix(std::string prefix);

bool canOverwriteFiles();
void canOverwriteFiles(bool permission);

bool canOverwriteDir();
void canOverwriteDir(bool permission);

bool canCreateDir();
void canCreateDir(bool permission);

bool bundleLibs();
void bundleLibs(bool on);

std::string destFolder();
void destFolder(std::string path);

void addFileToFix(std::string path);
int fileToFixAmount();
std::string fileToFix(const int n);

std::string inside_lib_path();
void inside_lib_path(std::string p);

void addSearchPath(std::string path);
int searchPathAmount();
std::string searchPath(const int n);

bool verboseOutput();
void verboseOutput(bool status);

} // namespace Settings

#endif
