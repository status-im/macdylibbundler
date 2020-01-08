#include "Utils.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

#include <unistd.h>

std::string filePrefix(const std::string& in)
{
    return in.substr(0, in.rfind('/')+1);
}

std::string stripPrefix(const std::string& in)
{
    return in.substr(in.rfind('/')+1);
}

std::string stripLSlash(const std::string& in)
{
    if (in[0] == '.' && in[1] == '/')
        return in.substr(2, in.size());
    return in;
}

std::string getFrameworkRoot(const std::string& in)
{
    return in.substr(0, in.find(".framework")+10);
}

std::string getFrameworkPath(const std::string& in)
{
    return in.substr(in.rfind(".framework/")+11);
}

void rtrim_in_place(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) {
        return !std::isspace(c);
    }).base(), s.end());
}

std::string rtrim(std::string s)
{
    rtrim_in_place(s);
    return s;
}

void tokenize(const std::string& str, const char* delim, std::vector<std::string>* tokens)
{
    std::vector<std::string>& out = *tokens;
    std::string delimiters(delim);

    // skip delimiters at beginning
    std::string::size_type end = str.find_first_not_of(delimiters, 0);
    // find first non-delimiter
    std::string::size_type start = str.find_first_of(delimiters, end);

    while (start != std::string::npos || end != std::string::npos) {
        out.push_back(str.substr(end, start - end));
        end = str.find_first_not_of(delimiters, start);
        start = str.find_first_of(delimiters, end);
    }
}

std::vector<std::string> lsDir(const std::string& path)
{
    std::string cmd = "/bin/ls " + path;
    std::string output = systemOutput(cmd);
    std::vector<std::string> files;
    tokenize(output, "\n", &files);
    return files;
}

bool fileExists(const std::string& filename)
{
    if (access(filename.c_str(), F_OK) != -1)
        return true;
    const std::string delims = " \f\n\r\t\v";
    std::string rtrimmed = filename.substr(0, filename.find_last_not_of(delims)+1);
    std::string ftrimmed = rtrimmed.substr(rtrimmed.find_first_not_of(delims));
    if (access(ftrimmed.c_str(), F_OK) != -1)
        return true;
    return false;
}

bool isRpath(const std::string& path)
{
    return path.find("@rpath") != std::string::npos || path.find("@loader_path") != std::string::npos;
}

std::string bundleExecutableName(const std::string& app_bundle_path)
{
    std::string cmd = std::string("/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' ");
                cmd += app_bundle_path;
                cmd += "Contents/Info.plist";
    return rtrim(systemOutput(cmd));
}

std::string systemOutput(const std::string& cmd)
{
    FILE* command_output = nullptr;
    char output[128];
    int amount_read = 1;
    std::string full_output;

    try {
        command_output = popen(cmd.c_str(), "r");
        if (command_output == nullptr)
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
        std::cerr << "An error occured while executing command " << cmd << std::endl;
        pclose(command_output);
        return "";
    }

    if (pclose(command_output) != 0)
        return "";
    return full_output;
}

void otool(const std::string& flags, const std::string& file, std::vector<std::string>& lines)
{
    std::string command = std::string("/usr/bin/otool ").append(flags).append(" ").append(file);
    std::string output = systemOutput(command);

    if (output.find("can't open file") != std::string::npos
            || output.find("No such file") != std::string::npos
            || output.find("at least one file must be specified") != std::string::npos
            || output.empty()) {
        std::cerr << "\n\n/!\\ ERROR: Cannot find file " << file << " to read its load commands\n";
        exit(1);
    }
    tokenize(output, "\n", &lines);
}

void parseLoadCommands(const std::string& file, const std::string& cmd, const std::string& value, std::vector<std::string>& lines)
{
    std::vector<std::string> raw_lines;
    otool("-l", file, raw_lines);

    bool searching = false;
    std::string cmd_line = std::string("cmd ").append(cmd);
    std::string value_line = std::string(value).append(" ");
    for (const auto& raw_line : raw_lines) {
        if (raw_line.find(cmd_line) != std::string::npos) {
            if (searching) {
                std::cerr << "\n\n/!\\ ERROR: Failed to find " << value << " before next cmd\n";
                exit(1);
            }
            searching = true;
        }
        else if (searching) {
            size_t start_pos = raw_line.find(value_line);
            if (start_pos == std::string::npos)
                continue;
            size_t start = start_pos + value.size() + 1; // exclude data label "|value| "
            size_t end = std::string::npos;
            if (value == "name" || value == "path") {
                size_t end_pos = raw_line.find(" (");
                if (end_pos == std::string::npos)
                    continue;
                end = end_pos - start;
            }
            lines.push_back(raw_line.substr(start, end));
            searching = false;
        }
    }
}

void parseLoadCommands(const std::string& file, const std::map<std::string,std::string>& cmds_values, std::map<std::string,std::vector<std::string>>& cmds_results)
{
    std::vector<std::string> raw_lines;
    otool("-l", file, raw_lines);

    for (const auto& cmd_value : cmds_values) {
        std::vector<std::string> lines;
        std::string cmd = cmd_value.first;
        std::string value = cmd_value.second;
        std::string cmd_line = std::string("cmd ").append(cmd);
        std::string value_line = std::string(value).append(" ");
        bool searching = false;
        for (const auto& raw_line : raw_lines) {
            if (raw_line.find(cmd_line) != std::string::npos) {
                if (searching) {
                    std::cerr << "\n\n/!\\ ERROR: Failed to find " << value << " before next cmd\n";
                    exit(1);
                }
                searching = true;
            } else if (searching) {
                size_t start_pos = raw_line.find(value_line);
                if (start_pos == std::string::npos)
                    continue;
                size_t start = start_pos + value.size() + 1; // exclude data label "|value| "
                size_t end = std::string::npos;
                if (value == "name" || value == "path") {
                    size_t end_pos = raw_line.find(" (");
                    if (end_pos == std::string::npos)
                        continue;
                    end = end_pos - start;
                }
                lines.push_back(raw_line.substr(start, end));
                searching = false;
            }
        }
        cmds_results[cmd] = lines;
    }
}

void createQtConf(std::string directory)
{
    std::string contents = "[Paths]\n"
                           "Plugins = PlugIns\n"
                           "Imports = Resources/qml\n"
                           "Qml2Imports = Resources/qml\n";
    if (directory[directory.size()-1] != '/')
        directory += "/";
    std::ofstream out(directory + "qt.conf");
    out << contents;
    out.close();
}
