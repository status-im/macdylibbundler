#include "Utils.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Dependency.h"
#include "Settings.h"

std::string filePrefix(std::string in)
{
    return in.substr(0, in.rfind("/")+1);
}

std::string stripPrefix(std::string in)
{
    return in.substr(in.rfind("/")+1);
}

std::string getFrameworkRoot(std::string in)
{
    return in.substr(0, in.find(".framework")+10);
}

std::string getFrameworkPath(std::string in)
{
    return in.substr(in.rfind(".framework/")+11);
}

std::string stripLSlash(std::string in)
{
    if (in[0] == '.' && in[1] == '/')
        in = in.substr(2, in.size());
    return in;
}

// trim from end (in place)
void rtrim_in_place(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) {
        return !std::isspace(c);
    }).base(), s.end());
}

// trim from end (copying)
std::string rtrim(std::string s)
{
    rtrim_in_place(s);
    return s;
}

std::string systemOutput(const std::string& cmd)
{
    FILE* command_output;
    char output[128];
    int amount_read = 1;
    std::string full_output;

    try {
        command_output = popen(cmd.c_str(), "r");
        if (command_output == NULL)
            throw;

        while (amount_read > 0) {
            amount_read = fread(output, 1, 127, command_output);
            if (amount_read <= 0) {
                break;
            }
            else {
                output[amount_read] = '\0';
                full_output += output;
            }
        }
    }
    catch (...) {
        std::cerr << "An error occured while executing command " << cmd << "\n";
        pclose(command_output);
        return "";
    }

    int return_value = pclose(command_output);
    if (return_value != 0)
        return "";

    return full_output;
}

int systemp(const std::string& cmd)
{
    if (!Settings::quietOutput()) {
        std::cout << "    " << cmd << "\n";
    }
    return system(cmd.c_str());
}

void tokenize(const std::string& str, const char* delim, std::vector<std::string>* vectorarg)
{
    std::vector<std::string>& tokens = *vectorarg;
    std::string delimiters(delim);

    // skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // find first "non-delimiter".
    std::string::size_type pos = str.find_first_of(delimiters, lastPos);

    while (pos != std::string::npos || lastPos != std::string::npos) {
        // found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

std::vector<std::string> lsDir(std::string path)
{
    std::string cmd = "ls " + path;
    std::string output = systemOutput(cmd);
    std::vector<std::string> files;
    tokenize(output, "\n", &files);
    return files;
}

bool fileExists(std::string filename)
{
    if (access(filename.c_str(), F_OK) != -1) {
        return true;
    }
    else {
        std::string delims = " \f\n\r\t\v";
        std::string rtrimmed = filename.substr(0, filename.find_last_not_of(delims)+1);
        std::string ftrimmed = rtrimmed.substr(rtrimmed.find_first_not_of(delims));
        if (access(ftrimmed.c_str(), F_OK) != -1)
            return true;
        else
            return false;
    }
}

bool isRpath(const std::string& path)
{
    return path.find("@rpath") == 0 || path.find("@loader_path") == 0;
}

std::string bundleExecutableName(const std::string& app_bundle_path)
{
    std::string cmd = "/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "
                    + app_bundle_path + "Contents/Info.plist";
    return systemOutput(cmd);
}

void changeId(std::string binary_file, std::string new_id)
{
    std::string command = std::string("install_name_tool -id ") + new_id + " " + binary_file;
    if (systemp(command) != 0) {
        std::cerr << "\n\nError: An error occured while trying to change identity of library " << binary_file << "\n";
        exit(1);
    }
}

void changeInstallName(std::string binary_file, std::string old_name, std::string new_name)
{
    std::string command = std::string("install_name_tool -change ") + old_name + " " + new_name + " " + binary_file;
    if (systemp(command) != 0) {
        std::cerr << "\n\nError: An error occured while trying to fix dependencies of " << binary_file << "\n";
        exit(1);
    }
}

void copyFile(std::string from, std::string to)
{
    bool overwrite = Settings::canOverwriteFiles();
    if (fileExists(to) && !overwrite) {
        std::cerr << "\n\nError: File " << to << " already exists. Remove it or enable overwriting (-of)\n";
        exit(1);
    }

    std::string overwrite_permission = std::string(overwrite ? "-f " : "-n ");

    // copy file/directory
    std::string command = std::string("cp -R ") + overwrite_permission + from + std::string(" ") + to;
    if (from != to && systemp(command) != 0) {
        std::cerr << "\n\nError: An error occured while trying to copy file " << from << " to " << to << "\n";
        exit(1);
    }

    // give file/directory write permission
    std::string command2 = std::string("chmod -R +w ") + to;
    if (systemp(command2) != 0) {
        std::cerr << "\n\nError: An error occured while trying to set write permissions on file " << to << "\n";
        exit(1);
    }
}

void deleteFile(std::string path, bool overwrite)
{
    std::string overwrite_permission = std::string(overwrite ? "-f " : " ");
    std::string command = std::string("rm -r ") + overwrite_permission + path;
    if (systemp(command) != 0) {
        std::cerr << "\n\nError: An error occured while trying to delete " << path << "\n";
        exit(1);
    }
}

void deleteFile(std::string path)
{
    bool overwrite = Settings::canOverwriteFiles();
    deleteFile(path, overwrite);
}

bool mkdir(std::string path)
{
    if (Settings::verboseOutput())
        std::cout << "* Creating directory " << path << "\n\n";
    std::string command = std::string("mkdir -p ") + path;
    if (systemp(command) != 0) {
        std::cerr << "\n/!\\ ERROR: An error occured while creating " + path + "\n";
        return false;
    }
    return true;
}

void createDestDir()
{
    std::string dest_folder = Settings::destFolder();
    if (Settings::verboseOutput())
        std::cout << "* Checking output directory " << dest_folder << "\n";

    bool dest_exists = fileExists(dest_folder);

    if (dest_exists && Settings::canOverwriteDir()) {
        std::cout << "* Erasing old output directory " << dest_folder << "\n";
        std::string command = std::string("rm -r ") + dest_folder;
        if (systemp(command) != 0) {
            std::cerr << "\n\n/!\\ ERROR: An error occured while attempting to overwrite dest folder\n";
            exit(1);
        }
        dest_exists = false;
    }

    if (!dest_exists) {
        if (Settings::canCreateDir()) {
            std::cout << "* Creating output directory " << dest_folder << "\n\n";
            if (!mkdir(dest_folder)) {
                std::cerr << "\n/!\\ ERROR: An error occured while creating " + dest_folder + "\n";
                exit(1);
            }
        }
        else {
            std::cerr << "\n\n/!\\ ERROR: Dest folder does not exist. Create it or pass the appropriate flag for automatic dest dir creation\n";
            exit(1);
        }
    }
}

std::string getUserInputDirForFile(const std::string& filename)
{
    const size_t searchPathCount = Settings::userSearchPathCount();
    for (size_t n=0; n<searchPathCount; ++n) {
        auto searchPath = Settings::userSearchPath(n);
        if (!searchPath.empty() && searchPath[searchPath.size()-1] != '/')
            searchPath += "/";
        if (!fileExists(searchPath+filename)) {
            continue;
        }
        else {
            if (!Settings::quietOutput()) {
                std::cerr << (searchPath+filename) << " was found\n"
                          << "/!\\ WARNING: dylibbundler MAY NOT CORRECTLY HANDLE THIS DEPENDENCY: Check the executable with 'otool -L'" << "\n";
            }
            return searchPath;
        }
    }

    while (true) {
        if (Settings::quietOutput())
            std::cerr << "\n/!\\ WARNING: Dependency " << filename << " has an incomplete name (location unknown)\n";
        std::cout << "\nPlease specify the directory where this file is located (or enter 'quit' to abort): ";
        fflush(stdout);

        std::string prefix;
        std::cin >> prefix;
        std::cout << std::endl;

        if (prefix.compare("quit") == 0 || prefix.compare("exit") == 0 || prefix.compare("abort") == 0)
            exit(1);

        if (!prefix.empty() && prefix[prefix.size()-1] != '/')
            prefix += "/";

        if (!fileExists(prefix+filename)) {
            std::cerr << (prefix+filename) << " does not exist. Try again...\n";
            continue;
        }
        else {
            std::cerr << (prefix+filename) << " was found\n"
                      << "/!\\ WARNINGS: dylibbundler MAY NOT CORRECTLY HANDLE THIS DEPENDENCY: Check the executable with 'otool -L'\n";
            Settings::addUserSearchPath(prefix);
            return prefix;
        }
    }
}

void initSearchPaths()
{
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
            Settings::addSearchPath(item);
        }
    }
}
