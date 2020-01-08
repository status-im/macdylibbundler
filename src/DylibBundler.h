#pragma once

#ifndef DYLIBBUNDLER_DYLIBBUNDLER_H
#define DYLIBBUNDLER_DYLIBBUNDLER_H

#include <string>
#include <vector>

void addDependency(const std::string& path, const std::string& dependent_file);
void collectDependenciesRpaths(const std::string& dependent_file);
void collectSubDependencies();
void changeLibPathsOnFile(const std::string& file_to_fix);
void fixRpathsOnFile(const std::string& original_file, const std::string& file_to_fix);
void bundleDependencies();
void bundleQtPlugins();

#endif
