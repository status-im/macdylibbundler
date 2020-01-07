#pragma once

#ifndef DYLIBBUNDLER_SETTINGS_H
#define DYLIBBUNDLER_SETTINGS_H

#include <cstdlib>
#include <map>
#include <string>
#include <utility>
#include <vector>

#ifndef __clang__
#include <sys/types.h>
#endif

class Settings {
public:
    Settings();
    virtual ~Settings();

    virtual bool isPrefixBundled(const std::string &prefix);
    virtual bool isPrefixIgnored(const std::string &prefix);
    virtual void ignorePrefix(std::string prefix);
    virtual std::string appBundle() { return app_bundle; }
    virtual void appBundle(std::string path);
    virtual bool appBundleProvided() { return !app_bundle.empty(); }
    
    virtual std::string destFolder() { return dest_path; }
    virtual void destFolder(std::string path);
    virtual std::string insideLibPath() { return inside_path; }
    virtual void insideLibPath(std::string p);
    
    virtual std::string executableFolder() { return app_bundle + "Contents/MacOS/"; }
    virtual std::string frameworksFolder() { return app_bundle + "Contents/Frameworks/"; }
    virtual std::string pluginsFolder() { return app_bundle + "Contents/PlugIns/"; }
    virtual std::string resourcesFolder() { return app_bundle + "Contents/Resources/"; }
    
    virtual std::vector<std::string> filesToFix() { return files; }
    virtual void addFileToFix(std::string path);
    virtual size_t filesToFixCount() { return files.size(); }

    virtual std::vector<std::string> searchPaths() { return search_paths; }
    virtual void addSearchPath(const std::string& path) { search_paths.push_back(path); }

    virtual std::vector<std::string> userSearchPaths() { return user_search_paths; }
    virtual void addUserSearchPath(const std::string& path) { user_search_paths.push_back(path); }

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

    virtual std::string getFullPath(const std::string& rpath) { return rpath_to_fullpath[rpath]; }
    virtual void rpathToFullPath(const std::string& rpath, const std::string& fullpath) { rpath_to_fullpath[rpath] = fullpath; }
    virtual bool rpathFound(const std::string& rpath) { return rpath_to_fullpath.find(rpath) != rpath_to_fullpath.end(); }

    virtual std::vector<std::string> getRpathsForFile(const std::string& file) { return rpaths_per_file[file]; }
    virtual void addRpathForFile(const std::string& file, const std::string& rpath) { rpaths_per_file[file].push_back(rpath); }
    virtual bool fileHasRpath(const std::string& file) { return rpaths_per_file.find(file) != rpaths_per_file.end(); }

protected:
    bool overwrite_files;
    bool overwrite_dir;
    bool create_dir;
    bool quiet_output;
    bool verbose_output;
    bool bundle_libs;
    bool bundle_frameworks;
    bool missing_prefixes;

    std::string dest_folder_str;
    std::string dest_folder_str_app;
    std::string dest_folder;
    std::string dest_path;

    std::string inside_path_str;
    std::string inside_path_str_app;
    std::string inside_path;

    std::string app_bundle;
    std::vector<std::string> prefixes_to_ignore;
    std::vector<std::string> search_paths;
    std::vector<std::string> files;

    std::vector<std::string> user_search_paths;
    std::map<std::string, std::string> rpath_to_fullpath;
    std::map<std::string, std::vector<std::string>> rpaths_per_file;
};

#endif
