#pragma once

#ifndef _depend_h_
#define _depend_h_

#include <string>
#include <vector>

class Dependency {
public:
    Dependency(std::string path, const std::string& dependent_file);

    [[nodiscard]] bool isFramework() const { return is_framework; }

    [[nodiscard]] std::string getPrefix() const { return prefix; }
    [[nodiscard]] std::string getOriginalFileName() const { return filename; }
    [[nodiscard]] std::string getOriginalPath() const { return prefix + filename; }

    [[nodiscard]] std::string getInstallPath() const;
    [[nodiscard]] std::string getInnerPath() const;

    void print() const;

    void addSymlink(const std::string& s);

    // Compare the given dependency with this one. If both refer to the same file,
    // merge both entries into one and return true.
    bool mergeIfSameAs(Dependency& dep2);

    void copyToAppBundle() const;
    void fixDependentFiles(const std::string& file) const;

private:
    bool is_framework;

    // origin
    std::string filename;
    std::string prefix;
    std::vector<std::string> symlinks;

    // installation
    std::string new_name;

    void print();
};

#endif
