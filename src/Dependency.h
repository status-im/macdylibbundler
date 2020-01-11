#pragma once

#ifndef MACDYLIBBUNDLER_DEPENDENCY_H
#define MACDYLIBBUNDLER_DEPENDENCY_H

#include <string>
#include <vector>

namespace macDylibBundler {

class DylibBundler;

class Dependency {
public:
    Dependency(std::string path, const std::string &dependent_file, DylibBundler *db);

    [[nodiscard]] bool IsFramework() const { return is_framework; }
    [[nodiscard]] std::string Prefix() const { return prefix; }
    [[nodiscard]] std::string OriginalFilename() const { return filename; }
    [[nodiscard]] std::string OriginalPath() const { return prefix + filename; }
    [[nodiscard]] std::string InnerPath() const;
    [[nodiscard]] std::string InstallPath() const;
    void AddSymlink(const std::string &path);
    // Compare the |dependency| with |this|. Merge entries if both refer to the same file.
    bool MergeIfIdentical(Dependency *dependency);

    void Print() const;

    void CopyToBundle() const;

    void FixDependentFile(const std::string &dependent_file) const;

private:
    bool is_framework;

    // origin
    std::string filename;
    std::string prefix;
    std::vector<std::string> symlinks;

    // installation
    std::string new_name;

    DylibBundler *db;
};

#endif

} // namespace macDylibBundler