#ifndef _depend_h_
#define _depend_h_

#include <string>
#include <vector>

class Dependency {
    // origin
    std::string filename;
    std::string prefix;
    std::vector<std::string> symlinks;

    // installation
    std::string new_name;
public:
    Dependency(std::string path);

    void print();

    std::string getOriginalFileName() const{ return filename; }
    std::string getOriginalPath() const{ return prefix+filename; }
    std::string getInstallPath();
    std::string getInnerPath();

    void addSymlink(std::string s);
    int getSymlinkAmount() const{ return symlinks.size(); }

    std::string getSymlink(const int i) const{ return symlinks[i]; }
    std::string getPrefix() const{ return prefix; }

    void copyYourself();
    void fixFileThatDependsOnMe(std::string file);

    // Compares the given dependency with this one. If both refer to the same file,
    // it returns true and merges both entries into one.
    bool mergeIfSameAs(Dependency& dep2);
};

#endif
