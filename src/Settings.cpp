#include "Settings.h"

#include <cstdlib>
#include <map>
#include <utility>

#include <sys/param.h>

#include "Utils.h"

namespace Settings {

bool overwrite_files = false;
bool overwrite_dir = false;
bool create_dir = false;
bool quiet_output = false;
bool verbose_output = false;
bool bundle_libs = true;
bool bundle_frameworks = false;

std::string dest_folder_str = "./libs/";
std::string dest_folder_str_app = "./Frameworks/";
std::string dest_folder = dest_folder_str;
std::string dest_path = dest_folder;

std::string inside_path_str = "@executable_path/../libs/";
std::string inside_path_str_app = "@executable_path/../Frameworks/";
std::string inside_path = inside_path_str;

std::string app_bundle;
std::string appBundle() { return app_bundle; }
void appBundle(std::string path)
{
    app_bundle = std::move(path);
    char buffer[PATH_MAX];
    if (realpath(app_bundle.c_str(), buffer))
        app_bundle = buffer;

    if (app_bundle[app_bundle.size()-1] != '/')
        app_bundle += "/"; // fix path if needed so it ends with '/'

    std::string bundle_executable_path = app_bundle + "Contents/MacOS/" + bundleExecutableName(app_bundle);
    if (realpath(bundle_executable_path.c_str(), buffer))
        bundle_executable_path = buffer;
    addFileToFix(bundle_executable_path);

    if (inside_path == inside_path_str)
        inside_path = inside_path_str_app;
    if (dest_folder == dest_folder_str)
        dest_folder = dest_folder_str_app;

    dest_path = app_bundle + "Contents/" + stripLSlash(dest_folder);
    if (realpath(dest_path.c_str(), buffer))
        dest_path = buffer;
    if (dest_path[dest_path.size()-1] != '/')
        dest_path += "/";
}
bool appBundleProvided() { return !app_bundle.empty(); }

std::string destFolder() { return dest_path; }
void destFolder(std::string path)
{
    dest_path = std::move(path);
    if (appBundleProvided())
        dest_path = app_bundle + "Contents/" + stripLSlash(dest_folder);
    char buffer[PATH_MAX];
    if (realpath(dest_path.c_str(), buffer))
        dest_path = buffer;
    if (dest_path[dest_path.size()-1] != '/')
        dest_path += "/";
}

std::string insideLibPath() { return inside_path; }
void insideLibPath(std::string p)
{
    inside_path = std::move(p);
    if (inside_path[inside_path.size()-1] != '/')
        inside_path += "/";
}

std::string executableFolder() { return app_bundle + "Contents/MacOS/"; }
std::string frameworksFolder() { return app_bundle + "Contents/Frameworks/"; }
std::string pluginsFolder() { return app_bundle + "Contents/PlugIns/"; }
std::string resourcesFolder() { return app_bundle + "Contents/Resources/"; }

std::vector<std::string> files;
void addFileToFix(std::string path)
{
    char buffer[PATH_MAX];
    if (realpath(path.c_str(), buffer))
        path = buffer;
    files.push_back(path);
}

std::vector<std::string> filesToFix() { return files; }
size_t filesToFixCount() { return files.size(); }

std::vector<std::string> prefixes_to_ignore;
void ignorePrefix(std::string prefix)
{
    if (prefix[prefix.size()-1] != '/')
        prefix += "/";
    prefixes_to_ignore.push_back(prefix);
}
bool isPrefixIgnored(const std::string& prefix)
{
    for (const auto& prefix_to_ignore : prefixes_to_ignore) {
        if (prefix == prefix_to_ignore)
            return true;
    }
    return false;
}

bool isPrefixBundled(const std::string& prefix)
{
    if (!bundle_frameworks && prefix.find(".framework") != std::string::npos)
        return false;
    if (prefix.find("@executable_path") != std::string::npos)
        return false;
    if (prefix.find("/usr/lib/") == 0)
        return false;
    if (prefix.find("/System/Library/") != std::string::npos)
        return false;
    if (isPrefixIgnored(prefix))
        return false;
    return true;
}

std::vector<std::string> search_paths;
std::vector<std::string> searchPaths() { return search_paths; }
void addSearchPath(const std::string& path) { search_paths.push_back(path); }

std::vector<std::string> user_search_paths;
std::vector<std::string> userSearchPaths() { return user_search_paths; }
void addUserSearchPath(const std::string& path) { user_search_paths.push_back(path); }

bool canCreateDir() { return create_dir; }
void canCreateDir(bool permission) { create_dir = permission; }

bool canOverwriteDir() { return overwrite_dir; }
void canOverwriteDir(bool permission) { overwrite_dir = permission; }

bool canOverwriteFiles() { return overwrite_files; }
void canOverwriteFiles(bool permission) { overwrite_files = permission; }

bool bundleLibs() { return bundle_libs; }
void bundleLibs(bool status) { bundle_libs = status; }

bool bundleFrameworks() { return bundle_frameworks; }
void bundleFrameworks(bool status) { bundle_frameworks = status; }

bool quietOutput() { return quiet_output; }
void quietOutput(bool status) { quiet_output = status; }

bool verboseOutput() { return verbose_output; }
void verboseOutput(bool status) { verbose_output = status; }

// if some libs are missing prefixes, then more stuff will be necessary to do
bool missing_prefixes = false;
bool missingPrefixes() { return missing_prefixes; }
void missingPrefixes(bool status) { missing_prefixes = status; }

std::map<std::string, std::string> rpath_to_fullpath;
std::string getFullPath(const std::string& rpath) { return rpath_to_fullpath[rpath]; }
void rpathToFullPath(const std::string& rpath, const std::string& fullpath) { rpath_to_fullpath[rpath] = fullpath; }
bool rpathFound(const std::string& rpath) { return rpath_to_fullpath.find(rpath) != rpath_to_fullpath.end(); }

std::map<std::string, std::vector<std::string>> rpaths_per_file;
std::vector<std::string> getRpathsForFile(const std::string& file) { return rpaths_per_file[file]; }
void addRpathForFile(const std::string& file, const std::string& rpath) { rpaths_per_file[file].push_back(rpath); }
bool fileHasRpath(const std::string& file) { return rpaths_per_file.find(file) != rpaths_per_file.end(); }

} // namespace Settings
