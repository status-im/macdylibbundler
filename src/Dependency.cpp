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

#include <stdlib.h>
#include <sys/param.h>

#include "DylibBundler.h"
// #include "ParallelForEach.h"
#include "Settings.h"
#include "Utils.h"

static inline std::string stripPrefix(std::string in)
{
    return in.substr(in.rfind("/")+1);
}

// trim from end (in place)
static inline void rtrim_in_place(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) {
        return !std::isspace(c);
    }).base(), s.end());
}

// trim from end (copying)
static inline std::string rtrim(std::string s)
{
    rtrim_in_place(s);
    return s;
}

// the paths to search for dylibs, store it globally to parse the environment variables only once
std::vector<std::string> paths;

// initialize the dylib search paths
void initSearchPaths()
{
    //Check the same paths the system would search for dylibs
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

// if some libs are missing prefixes, this will be set to true
// more stuff will then be necessary to do
bool missing_prefixes = false;

Dependency::Dependency(std::string path)
{
    char original_file_buffer[PATH_MAX];
    std::string original_file;

    if (isRpath(path)) {
        original_file = searchFilenameInRpaths(path);
    }
    else if (!realpath(rtrim(path).c_str(), original_file_buffer)) {
        std::cerr << "\n/!\\ WARNING : Cannot resolve path '" << path << "'" << std::endl;
        original_file = path;
    }
    else {
        original_file = original_file_buffer;
    }

    // check if given path is a symlink
    if (original_file != rtrim(path)) {
        filename = stripPrefix(original_file);
        prefix = original_file.substr(0, original_file.rfind("/")+1);
        addSymlink(path);
    }
    else {
        filename = stripPrefix(path);
        prefix = path.substr(0, path.rfind("/")+1);
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
                std::cout << "FOUND " << filename << " in " << paths[i] << std::endl;
                prefix = paths[i];
                missing_prefixes = true; // the prefix was missing
                break;
            }
        }
    }

    // if the location is still unknown, ask the user for search path
    if (!Settings::isPrefixIgnored(prefix) && (prefix.empty() || !fileExists(prefix+filename))) {
        std::cerr << "\n/!\\ WARNING : Library " << filename << " has an incomplete name (location unknown)" << std::endl;
        missing_prefixes = true;
        paths.push_back(getUserInputDirForFile(filename));
    }

    // new_name  = filename.substr(0, filename.find(".")) + ".dylib";
    new_name = filename;
}

void Dependency::print()
{
    const int symamount = symlinks.size();
    std::cout << std::endl << " * " << filename << " from " << prefix << std::endl;
    for (int n=0; n<symamount; ++n)
        std::cout << "     symlink --> " << symlinks[n] << std::endl;;
}

std::string Dependency::getInstallPath()
{
    return Settings::destFolder() + new_name;
}

std::string Dependency::getInnerPath()
{
    return Settings::inside_lib_path() + new_name;
}

void Dependency::addSymlink(std::string s)
{
    // calling std::find on this vector is not near as slow as an extra invocation of install_name_tool
    if (std::find(symlinks.begin(), symlinks.end(), s) == symlinks.end())
        symlinks.push_back(s);
}

// compare the given Dependency with this one.
// if both refer to the same file, merge both entries into one
bool Dependency::mergeIfSameAs(Dependency& dep2)
{
    if (dep2.getOriginalFileName().compare(filename) == 0) {
        const int samount = getSymlinkAmount();
        for (int n=0; n<samount; ++n)
            dep2.addSymlink(getSymlink(n));
        return true;
    }
    return false;
}

void Dependency::copyYourself()
{
    copyFile(getOriginalPath(), getInstallPath());
    // Fix the lib's inner name
    std::string command = std::string("install_name_tool -id ") + getInnerPath() + " " + getInstallPath();
    if (systemp(command) != 0) {
        std::cerr << "\n\nError : An error occured while trying to change identity of library " << getInstallPath() << std::endl;
        exit(1);
    }
}

void Dependency::fixFileThatDependsOnMe(std::string file_to_fix)
{
    // for main lib file
    std::string command = std::string("install_name_tool -change ") + getOriginalPath() + " " + getInnerPath() + " " + file_to_fix;
    if (systemp(command) != 0) {
        std::cerr << "\n\nError : An error occured while trying to fix dependencies of " << file_to_fix << std::endl;
        exit(1);
    }

    // for symlinks
    const int symamount = symlinks.size();
    for (int n=0; n<symamount; ++n) {
        command = std::string("install_name_tool -change ") + symlinks[n] + " " + getInnerPath() + " " + file_to_fix;
        if (systemp(command) != 0) {
            std::cerr << "\n\nError : An error occured while trying to fix dependencies of " << file_to_fix << std::endl;
            exit(1);
        }
    }

    // FIXME - hackish
    if (missing_prefixes) {
        // for main lib file
        command = std::string("install_name_tool -change ") + filename + " " + getInnerPath() + " " + file_to_fix;
        if (systemp(command) != 0) {
            std::cerr << "\n\nError : An error occured while trying to fix dependencies of " << file_to_fix << std::endl;
            exit(1);
        }

        // for symlinks
        for (int n=0; n<symamount; ++n) {
            command = std::string("install_name_tool -change ") + symlinks[n] + " " + getInnerPath() + " " + file_to_fix;
            if (systemp(command) != 0) {
                std::cerr << "\n\nError : An error occured while trying to fix dependencies of " << file_to_fix << std::endl;
                exit(1);
            }
        }
    }
}
