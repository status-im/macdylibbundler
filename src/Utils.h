/*
The MIT License (MIT)

Copyright (c) 2014 Marianne Gagnon

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
 */


#ifndef _utils_h_
#define _utils_h_

#include <string>
#include <vector>

class Library;

std::string filePrefix(std::string in);
std::string stripPrefix(std::string in);

std::string getFrameworkRoot(std::string in);
std::string getFrameworkPath(std::string in);

std::string stripLSlash(std::string in);

// trim from end (in place)
void rtrim_in_place(std::string& s);
// trim from end (copying)
std::string rtrim(std::string s);

void tokenize(const std::string& str, const char* delimiters, std::vector<std::string>*);
bool fileExists( std::string filename );

void copyFile(std::string from, std::string to);

void deleteFile(std::string path, bool overwrite);
void deleteFile(std::string path);

std::vector<std::string> lsDir(const std::string& path);
bool mkdir(const std::string& path);

// executes a command in the native shell and returns output in string
std::string system_get_output(std::string cmd);

// like 'system', runs a command on the system shell, but also prints the command to stdout.
int systemp(const std::string& cmd);

std::string getUserInputDirForFile(const std::string& filename, const std::string& dependent_file);

std::string bundleExecutableName(const std::string& app_bundle_path);

void changeId(std::string binary_file, std::string new_id);
void changeInstallName(std::string binary_file, std::string old_name, std::string new_name);

// check the same paths the system would search for dylibs
void initSearchPaths();

void createQtConf(std::string directory);

#endif
