#pragma once

#ifndef DYLIBBUNDLER_DYLIBBUNDLER_H
#define DYLIBBUNDLER_DYLIBBUNDLER_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "Settings.h"

class Dependency;

class DylibBundler : public Settings {
public:
    DylibBundler();
    ~DylibBundler() override;

    void addDependency(const std::string &path, const std::string &dependent_file);
    void collectDependenciesRpaths(const std::string &dependent_file);
    void collectSubDependencies();
    void changeLibPathsOnFile(const std::string &file_to_fix);
    void fixRpathsOnFile(const std::string &original_file, const std::string &file_to_fix);
    void bundleDependencies();
    void bundleQtPlugins();

    std::string getUserInputDirForFile(const std::string& filename, const std::string& dependent_file);
    std::string searchFilenameInRpaths(const std::string& rpath_file, const std::string& dependent_file);
    std::string searchFilenameInRpaths(const std::string& rpath_file);

    // check the same paths the system would search for dylibs
    void initSearchPaths();
    void createDestDir();

    void changeId(const std::string& binary_file, const std::string& new_id);
    void changeInstallName(const std::string& binary_file, const std::string& old_name, const std::string& new_name);

    void copyFile(const std::string& from, const std::string& to);
    void deleteFile(const std::string& path, bool overwrite);
    void deleteFile(const std::string& path);
    bool mkdir(const std::string& path);

    // run a command in the system shell (like 'system') but also print the command to stdout
    int systemp(const std::string& cmd);

private:
    std::vector<Dependency*> deps;
    std::map<std::string, std::vector<Dependency*>> deps_per_file;
    std::map<std::string, bool> deps_collected;
    std::set<std::string> frameworks;
    std::set<std::string> rpaths;
    std::map<std::string, bool> rpaths_collected;
    bool qt_plugins_called;
};

#endif
