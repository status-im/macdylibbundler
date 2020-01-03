#include "Settings.h"
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
bool appBundleProvided() { return !app_bundle.empty(); }
std::string appBundle() { return app_bundle; }
void appBundle(std::string path) {
    app_bundle = path;
    if (app_bundle[app_bundle.size()-1] != '/')
        app_bundle += "/"; // fix path if needed so it ends with '/'

    std::string cmd = "/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' ";
    cmd += app_bundle + "Contents/Info.plist";
    std::string bundle_executable = systemOutput(cmd);

    addFileToFix(app_bundle + "Contents/MacOS/" + bundle_executable);

    if (inside_path == inside_path_str)
        inside_path = inside_path_str_app;
    if (dest_folder == dest_folder_str)
        dest_folder = dest_folder_str_app;

    dest_path = app_bundle + "Contents/" + dest_folder;
    char buffer[PATH_MAX];
    if (realpath(dest_path.c_str(), buffer))
        dest_path = buffer;
    if (dest_path[dest_path.size()-1] != '/')
        dest_path += "/";
}

std::string destFolder() { return dest_path; }
void destFolder(std::string path)
{
    if (path[path.size()-1] != '/')
        path += "/";
    dest_folder = path;
    if (appBundleProvided()) {
        char buffer[PATH_MAX];
        std::string dest_path = app_bundle + "Contents/" + path;
        if (realpath(dest_path.c_str(), buffer))
            dest_path = buffer;
        if (dest_path[dest_path.size()-1] != '/')
            dest_path += "/";
        dest_folder = dest_path;
    }
}

std::string executableFolder() { return app_bundle + "Contents/MacOS/"; }
std::string frameworksFolder() { return app_bundle + "Contents/Frameworks/"; }
std::string pluginsFolder() { return app_bundle + "Contents/PlugIns/"; }
std::string resourcesFolder() { return app_bundle + "Contents/Resources/"; }

std::vector<std::string> files;
void addFileToFix(std::string path) { files.push_back(path); }
std::string fileToFix(const int n) { return files[n]; }
std::vector<std::string> filesToFix() { return files; }
size_t filesToFixCount() { return files.size(); }

std::string insideLibPath() { return inside_path; }
void insideLibPath(std::string p)
{
    inside_path = p;
    // fix path if needed so it ends with '/'
    if (inside_path[inside_path.size()-1] != '/')
        inside_path += "/";
}

std::vector<std::string> prefixes_to_ignore;
void ignorePrefix(std::string prefix)
{
    if (prefix[prefix.size()-1] != '/')
        prefix += "/";
    prefixes_to_ignore.push_back(prefix);
}
bool isPrefixIgnored(std::string prefix)
{
    for (size_t n=0; n<prefixes_to_ignore.size(); n++) {
        if (prefix.compare(prefixes_to_ignore[n]) == 0)
            return true;
    }
    return false;
}

bool isPrefixBundled(std::string prefix)
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

std::vector<std::string> searchPaths;
void addSearchPath(std::string path) { searchPaths.push_back(path); }
size_t searchPathCount() { return searchPaths.size(); }
std::string searchPath(const int n) { return searchPaths[n]; }

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

} // namespace Settings
