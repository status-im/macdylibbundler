#pragma once

#ifndef MACDYLIBBUNDLER_SETTINGS_H
#define MACDYLIBBUNDLER_SETTINGS_H

#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>

#ifndef __clang__
#include <sys/types.h>
#endif

using namespace std;
namespace macDylibBundler {

class Settings {
public:
    Settings();

    virtual ~Settings();

    virtual bool isPrefixBundled(const string &prefix);
    virtual bool isPrefixIgnored(const string &prefix);
    virtual void ignorePrefix(string prefix);

    virtual string appBundle() { return app_bundle; }
    virtual void appBundle(string path);
    virtual bool appBundleProvided() { return !app_bundle.empty(); }

    virtual string destFolder() { return dest_path; }
    virtual void destFolder(string path);
    virtual string insideLibPath() { return inside_path; }
    virtual void insideLibPath(string p);

    virtual string executableFolder() { return app_bundle + "Contents/MacOS/"; }
    virtual string frameworksFolder() { return app_bundle + "Contents/Frameworks/"; }
    virtual string pluginsFolder() { return app_bundle + "Contents/PlugIns/"; }
    virtual string resourcesFolder() { return app_bundle + "Contents/Resources/"; }

    virtual vector<string> filesToFix() { return files; }
    virtual void addFileToFix(string path);

    virtual vector<string> searchPaths() { return search_paths; }
    virtual void addSearchPath(const string &path) { search_paths.push_back(path); }
    virtual vector<string> userSearchPaths() { return user_search_paths; }
    virtual void addUserSearchPath(const string &path) { user_search_paths.push_back(path); }

    virtual bool canCreateDir() { return create_dir; }
    virtual void canCreateDir(bool permission) { create_dir = permission; }
    virtual bool canOverwriteDir() { return overwrite_dir; }
    virtual void canOverwriteDir(bool permission) { overwrite_dir = permission; }
    virtual bool canOverwriteFiles() { return overwrite_files; }
    virtual void canOverwriteFiles(bool permission) { overwrite_files = permission; }
    virtual bool bundleLibs() { return bundle_libs; }
    virtual void bundleLibs(bool status) { bundle_libs = status; }
    virtual bool bundleFrameworks() { return bundle_frameworks; }
    virtual void bundleFrameworks(bool status) { bundle_frameworks = status; }
    virtual bool quietOutput() { return quiet_output; }
    virtual void quietOutput(bool status) { quiet_output = status; }
    virtual bool verboseOutput() { return verbose_output; }
    virtual void verboseOutput(bool status) { verbose_output = status; }

    virtual bool missingPrefixes() { return missing_prefixes; }
    virtual void missingPrefixes(bool status) { missing_prefixes = status; }

    virtual string getFullPath(const string &rpath) { return rpath_to_fullpath[rpath]; }
    virtual void rpathToFullPath(const string &rpath, const string &fullpath)
    {
        rpath_to_fullpath[rpath] = fullpath;
    }
    virtual bool rpathFound(const string &rpath)
    {
        return rpath_to_fullpath.find(rpath) != rpath_to_fullpath.end();
    }

    virtual vector<string> getRpathsForFile(const string &file)
    {
        return rpaths_per_file[file];
    }
    virtual void addRpathForFile(const string &file, const string &rpath)
    {
        rpaths_per_file[file].push_back(rpath);
    }
    virtual bool fileHasRpath(const string &file)
    {
        return rpaths_per_file.find(file) != rpaths_per_file.end();
    }

private:
    bool overwrite_files;
    bool overwrite_dir;
    bool create_dir;
    bool quiet_output;
    bool verbose_output;
    bool bundle_libs;
    bool bundle_frameworks;
    bool missing_prefixes;

    string dest_folder_str;
    string dest_folder_str_app;
    string dest_folder;
    string dest_path;

    string inside_path_str;
    string inside_path_str_app;
    string inside_path;

    string app_bundle;
    vector<string> prefixes_to_ignore;
    vector<string> search_paths;
    vector<string> files;

    vector<string> user_search_paths;
    map<string, string> rpath_to_fullpath;
    map<string, vector<string>> rpaths_per_file;
};

} // namespace macDylibBundler

#endif
