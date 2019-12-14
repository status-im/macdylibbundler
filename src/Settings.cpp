#include "Settings.h"

namespace Settings {

bool overwrite_files = false;
bool overwrite_dir = false;
bool create_dir = false;
bool verbose_output = true;

bool canOverwriteFiles() { return overwrite_files; }
bool canOverwriteDir() { return overwrite_dir; }
bool canCreateDir() { return create_dir; }

void canOverwriteFiles(bool permission) { overwrite_files = permission; }
void canOverwriteDir(bool permission) { overwrite_dir = permission; }
void canCreateDir(bool permission) { create_dir = permission; }

bool bundleLibs_bool = false;
bool bundleLibs() { return bundleLibs_bool; }
void bundleLibs(bool on) { bundleLibs_bool = on; }

std::string dest_folder_str = "./Frameworks/";
std::string destFolder() { return dest_folder_str; }
void destFolder(std::string path)
{
    dest_folder_str = path;
    // fix path if needed so it ends with '/'
    if (dest_folder_str[dest_folder_str.size()-1] != '/')
        dest_folder_str += "/";
}

std::vector<std::string> files;
void addFileToFix(std::string path) { files.push_back(path); }
int fileToFixAmount() { return files.size(); }
std::string fileToFix(const int n) { return files[n]; }
std::vector<std::string> filesToFix() { return files; }

std::string inside_path_str = "@executable_path/../Frameworks/";
std::string inside_lib_path() { return inside_path_str; }
void inside_lib_path(std::string p)
{
    inside_path_str = p;
    // fix path if needed so it ends with '/'
    if (inside_path_str[inside_path_str.size()-1] != '/')
        inside_path_str += "/";
}

std::vector<std::string> prefixes_to_ignore;
void ignore_prefix(std::string prefix)
{
    if (prefix[prefix.size()-1] != '/')
        prefix += "/";
    prefixes_to_ignore.push_back(prefix);
}

bool isPrefixIgnored(std::string prefix)
{
    const int prefix_amount = prefixes_to_ignore.size();
    for (int n=0; n<prefix_amount; n++) {
        if (prefix.compare(prefixes_to_ignore[n]) == 0)
            return true;
    }

    return false;
}

bool isPrefixBundled(std::string prefix)
{
    if (prefix.find(".framework") != std::string::npos)
        return false;
    if (prefix.find("@executable_path") != std::string::npos)
        return false;
    if (prefix.compare("/usr/lib/") == 0)
        return false;
    if (prefix.compare("/System/Library/") == 0)
        return false;
    if (isPrefixIgnored(prefix))
        return false;

    return true;
}

std::vector<std::string> searchPaths;
void addSearchPath(std::string path) { searchPaths.push_back(path); }
int searchPathAmount() { return searchPaths.size(); }
std::string searchPath(const int n) { return searchPaths[n]; }

bool verboseOutput() { return verbose_output; }
void verboseOutput(bool status) { verbose_output = status; }

} // namespace Settings
