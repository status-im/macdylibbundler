#ifndef _DYLIB_BUNDLER_H_
#define _DYLIB_BUNDLER_H_

#include <string>
#include <vector>

void addDependency(std::string path, std::string dependent_file);

std::string searchFilenameInRpaths(const std::string& rpath_file, const std::string& dependent_file);
std::string searchFilenameInRpaths(const std::string& rpath_file);

// fill |lines| with dependencies of |dependent_file|
void collectDependencies(const std::string& dependent_file, std::vector<std::string>& lines);
void collectDependenciesForFile(const std::string& dependent_file, std::vector<std::string>& lines);
void collectDependenciesForFile(const std::string& dependent_file);

void collectRpaths(const std::string& filename);
void collectRpathsForFilename(const std::string& filename);

// recursively collect each dependency's dependencies
void collectSubDependencies();

void changeLibPathsOnFile(std::string file_to_fix);
void fixRpathsOnFile(const std::string& original_file, const std::string& file_to_fix);

void doneWithDeps_go();

void copyQtPlugins();

#endif
