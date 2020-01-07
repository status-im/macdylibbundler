#pragma once

#ifndef DYLIBBUNDLER_DEPENDENCY_H
#define DYLIBBUNDLER_DEPENDENCY_H

#include <string>
#include <vector>

class DylibBundler;

class Dependency {
public:
    Dependency(std::string path, const std::string& dependent_file, DylibBundler* db);

    [[nodiscard]] bool IsFramework() const { return is_framework; }

    [[nodiscard]] std::string Prefix() const { return prefix; }
    [[nodiscard]] std::string OriginalFilename() const { return filename; }
    [[nodiscard]] std::string OriginalPath() const { return prefix + filename; }

    [[nodiscard]] std::string InnerPath() const;
    [[nodiscard]] std::string InstallPath() const;

    void AddSymlink(const std::string& path);

    // Compare the given dependency with this one. If both refer to the same file,
    // merge both entries into one and return true.
    bool MergeIfIdentical(Dependency* dependency);

    void CopyToBundle() const;
    void FixDependentFile(const std::string& dependent_file) const;

    void Print() const;

private:
    bool is_framework;

    // origin
    std::string filename;
    std::string prefix;
    std::vector<std::string> symlinks;

    // installation
    std::string new_name;

    DylibBundler* db;
};

#endif
