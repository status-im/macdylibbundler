#ifndef _depend_h_
#define _depend_h_

#include <string>
#include <vector>

class Dependency {
public:
    Dependency(std::string path, std::string dependent_file);

    std::string getOriginalFileName() const { return filename; }
    std::string getOriginalPath() const { return prefix + filename; }

    std::string getInstallPath();
    std::string getInnerPath();

    bool isFramework() { return is_framework; }

    void addSymlink(std::string s);
    size_t symlinksCount() const { return symlinks.size(); }

    std::string getSymlink(int i) const { return symlinks[i]; }
    std::string getPrefix() const { return prefix; }

    // Compares the given dependency with this one. If both refer to the same file,
    // it returns true and merges both entries into one.
    bool mergeIfSameAs(Dependency& dep2);

    void print();

    void copyYourself();
    void fixFileThatDependsOnMe(std::string file);

private:
    bool is_framework;
    // origin
    std::string filename;
    std::string prefix;
    std::vector<std::string> symlinks;
    // installation
    std::string new_name;
};

#endif
