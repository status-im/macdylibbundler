#include "Utils.h"
#include "Dependency.h"
#include "Settings.h"
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <cstdio>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

void tokenize(const string& str, const char* delim, vector<string>* vectorarg)
{
    vector<string>& tokens = *vectorarg;

    string delimiters(delim);

    // skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);

    // find first "non-delimiter".
    string::size_type pos = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos) {
        // found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));

        // skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);

        // find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}



bool fileExists( std::string filename )
{
    if (access(filename.c_str(), F_OK) != -1) {
        return true; // file exists
    }
    else {
        std::string delims = " \f\n\r\t\v";
        std::string rtrimmed = filename.substr(0, filename.find_last_not_of(delims) + 1);
        std::string ftrimmed = rtrimmed.substr(rtrimmed.find_first_not_of(delims));
        if (access( ftrimmed.c_str(), F_OK ) != -1)
            return true;
        else
            return false; // file doesn't exist
    }
}

void copyFile(string from, string to)
{
    bool overwrite = Settings::canOverwriteFiles();
    if (!overwrite) {
        if (fileExists(to)) {
            cerr << "\n\nError : File " << to.c_str() << " already exists. Remove it or enable overwriting." << endl;
            exit(1);
        }
    }

    string overwrite_permission = string(overwrite ? "-f " : "-n ");

    // copy file to local directory
    string command = string("cp ") + overwrite_permission + from + string(" ") + to;
    if (from != to && systemp(command) != 0) {
        cerr << "\n\nError : An error occured while trying to copy file " << from << " to " << to << endl;
        exit(1);
    }

    // give it write permission
    string command2 = string("chmod +w ") + to;
    if (systemp(command2) != 0) {
        cerr << "\n\nError : An error occured while trying to set write permissions on file " << to << endl;
        exit(1);
    }
}

std::string system_get_output(std::string cmd)
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
        std::cerr << "An error occured while executing command " << cmd.c_str() << std::endl;
        pclose(command_output);
        return "";
    }

    int return_value = pclose(command_output);
    if (return_value != 0)
        return "";

    return full_output;
}

int systemp(std::string& cmd)
{
    if (Settings::verboseOutput()) {
        std::cout << "    " << cmd.c_str() << std::endl;
    }
    return system(cmd.c_str());
}

std::string getUserInputDirForFile(const std::string& filename)
{
    const int searchPathAmount = Settings::searchPathAmount();
    for (int n=0; n<searchPathAmount; n++) {
        auto searchPath = Settings::searchPath(n);
        if(!searchPath.empty() && searchPath[searchPath.size()-1] != '/')
            searchPath += "/";

        if (!fileExists(searchPath+filename)) {
            continue;
        } else {
            std::cerr << (searchPath+filename) << " was found.\n"
                      << "/!\\ DylibBundler MAY NOT CORRECTLY HANDLE THIS DEPENDENCY: "
                      << "Check the executable with 'otool -L'" << std::endl;
            return searchPath;
        }
    }

    while (true)
    {
        std::cout << "Please specify the directory where this library is located (or enter 'quit' to abort): ";  fflush(stdout);

        std::string prefix;
        std::cin >> prefix;
        std::cout << std::endl;

        if (prefix.compare("quit") == 0)
            exit(1);

        if (!prefix.empty() && prefix[prefix.size()-1] != '/')
            prefix += "/";

        if (!fileExists( prefix+filename)) {
            std::cerr << (prefix+filename) << " does not exist. Try again" << std::endl;
            continue;
        }
        else {
            std::cerr << (prefix+filename) << " was found.\n"
                      << "/!\\ DylibBundler MAY NOT CORRECTLY HANDLE THIS DEPENDENCY: "
                      << "Check the executable with 'otool -L'" << std::endl;
            return prefix;
        }
    }
}
