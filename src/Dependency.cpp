#include "Dependency.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <locale>
#include <sstream>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "DylibBundler.h"
#include "Settings.h"
#include "Utils.h"

// the paths to search for dylibs, store it globally to parse the environment variables only once
std::vector<std::string> paths;

// initialize the dylib search paths
void initSearchPaths()
{
    std::cout << "**** RUNNING initSearchPaths() ****\n";
    // check the same paths the system would search for dylibs
    std::string searchPaths;
    char *dyldLibPath = std::getenv("DYLD_LIBRARY_PATH");
    if (dyldLibPath != 0)
        searchPaths = dyldLibPath;
    dyldLibPath = std::getenv("DYLD_FALLBACK_FRAMEWORK_PATH");
    if (dyldLibPath != 0) {
        if (!searchPaths.empty() && searchPaths[searchPaths.size()-1] != ':')
            searchPaths += ":";
        searchPaths += dyldLibPath;
    }
    dyldLibPath = std::getenv("DYLD_FALLBACK_LIBRARY_PATH");
    if (dyldLibPath != 0) {
        if (!searchPaths.empty() && searchPaths[searchPaths.size()-1] != ':')
            searchPaths += ":";
        searchPaths += dyldLibPath;
    }
    if (!searchPaths.empty()) {
        std::stringstream ss(searchPaths);
        std::string item;
        while (std::getline(ss, item, ':')) {
            if (item[item.size()-1] != '/')
                item += "/";
            paths.push_back(item);
        }
    }
}

// if some libs are missing prefixes, then more stuff will be necessary to do
bool missing_prefixes = false;

Dependency::Dependency(std::string path) : is_framework(false)
{
    char original_file_buffer[PATH_MAX];
    std::string original_file;
    std::string warning_msg;

    if (isRpath(path)) {
        original_file = searchFilenameInRpaths(path);
    }
    else if (realpath(rtrim(path).c_str(), original_file_buffer)) {
        original_file = original_file_buffer;
    }
    else {
        warning_msg = "\n/!\\ WARNING: Cannot resolve path '" + path + "'\n";
        original_file = path;
    }

    // check if given path is a symlink
    if (original_file != rtrim(path)) {
        filename = stripPrefix(original_file);
        prefix = filePrefix(original_file);
        addSymlink(path);
    }
    else {
        filename = stripPrefix(path);
        prefix = filePrefix(path);
    }

    // check if this dependency is in /usr/lib, /System/Library, or in ignored list
    if (!Settings::isPrefixBundled(prefix))
        return;

    if (getOriginalPath().find(".framework") != std::string::npos) {
        is_framework = true;
        std::string framework_root = getFrameworkRoot(original_file);
        std::string framework_path = getFrameworkPath(original_file);
        std::string framework_name = stripPrefix(framework_root);
        filename = framework_name + "/" + framework_path;
        prefix = filePrefix(framework_root);
        if (Settings::verboseOutput()) {
            std::cout << "framework root: " << framework_root << std::endl;
            std::cout << "framework path: " << framework_path << std::endl;
            std::cout << "framework name: " << framework_name << std::endl;
        }
    }

    // check if the lib is in a known location
    if (!prefix.empty() && prefix[prefix.size()-1] != '/')
        prefix += "/";

    if (prefix.empty() || !fileExists(prefix+filename)) {
        // the paths contains at least /usr/lib so if it is empty we have not initialized it
        if (paths.empty())
            initSearchPaths();

        // check if file is contained in one of the paths
        for (size_t i=0; i<paths.size(); ++i) {
            if (fileExists(paths[i]+filename)) {
                warning_msg += "FOUND " + filename + " in " + paths[i] + "\n";
                prefix = paths[i];
                missing_prefixes = true;
                break;
            }
        }
    }

    if (!Settings::quietOutput())
        std::cout << warning_msg;

    // if the location is still unknown, ask the user for search path
    if (!Settings::isPrefixIgnored(prefix) && (prefix.empty() || !fileExists(prefix+filename))) {
        if (!Settings::quietOutput())
            std::cerr << "\n/!\\ WARNING: Library " << filename << " has an incomplete name (location unknown)\n";
        if (Settings::verboseOutput()) {
            std::cout << "path: " << (prefix+filename) << std::endl;
            std::cout << "prefix: " << prefix << std::endl;
        }
        missing_prefixes = true;
        paths.push_back(getUserInputDirForFile(filename));
    }

    new_name = filename;
}

std::string Dependency::getInstallPath()
{
    return Settings::destFolder() + new_name;
}

std::string Dependency::getInnerPath()
{
    return Settings::insideLibPath() + new_name;
}

void Dependency::addSymlink(std::string s)
{
    // calling std::find on this vector is not as slow as an extra invocation of install_name_tool
    if (std::find(symlinks.begin(), symlinks.end(), s) == symlinks.end())
        symlinks.push_back(s);
}

// compare given Dependency with this one. if both refer to the same file, merge into one entry.
bool Dependency::mergeIfSameAs(Dependency& dep2)
{
    if (dep2.getOriginalFileName().compare(filename) == 0) {
        for (size_t n=0; n<symlinks.size(); ++n)
            dep2.addSymlink(symlinks[n]);
        return true;
    }
    return false;
}

void Dependency::print()
{
    std::cout << "\n * " << filename << " from " << prefix << "\n";

    for (size_t n=0; n<symlinks.size(); ++n)
        std::cout << "     symlink --> " << symlinks[n] << "\n";;
}

void Dependency::copyYourself()
{
    std::string original_path = getOriginalPath();
    std::string dest_path = getInstallPath();
    std::string inner_path = getInnerPath();
    std::string install_path = getInstallPath();

    if (is_framework) {
        original_path = getFrameworkRoot(original_path);
        dest_path = Settings::destFolder() + stripPrefix(original_path);
    }

    if (Settings::verboseOutput()) {
        std::cout << "original path: " << original_path << std::endl;
        std::cout << "inner path:    " << inner_path << std::endl;
        std::cout << "dest_path:     " << dest_path << std::endl;
        std::cout << "install path:  " << install_path << std::endl;
    }

    copyFile(original_path, dest_path);

    if (is_framework) {
        std::string headers_path = dest_path + std::string("/Headers");
        std::string headers_realpath = headers_path;
        char buffer[PATH_MAX];

        if (realpath(rtrim(headers_path).c_str(), buffer))
            headers_realpath = buffer;

        if (Settings::verboseOutput())
            std::cout << "headers path:  " << headers_realpath << std::endl;

        deleteFile(headers_path, true);
        deleteFile(headers_realpath, true);
    }

    // fix the lib's inner name
    changeId(install_path, inner_path);
}

void Dependency::fixFileThatDependsOnMe(std::string file_to_fix)
{
    // for main lib file
    changeInstallName(file_to_fix, getOriginalPath(), getInnerPath());
    // for symlinks
    for (size_t n=0; n<symlinks.size(); ++n)
        changeInstallName(file_to_fix, symlinks[n], getInnerPath());

    // FIXME - hackish
    if (missing_prefixes) {
        // for main lib file
        changeInstallName(file_to_fix, filename, getInnerPath());
        // for symlinks
        for (size_t n=0; n<symlinks.size(); ++n)
            changeInstallName(file_to_fix, symlinks[n], getInnerPath());
    }
}
