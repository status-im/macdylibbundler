#include "Dependency.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>

#include <sys/param.h>
#ifndef __clang__
#include <sys/types.h>
#endif

#include "Utils.h"

Dependency::Dependency(std::string path, const std::string& dependent_file, DylibBundler* db)
        : is_framework(false),
          db_(db)

{
    char buffer[PATH_MAX];
    rtrim_in_place(path);
    std::string original_file;
    std::string warning_msg;

    if (isRpath(path)) {
        original_file = db_->searchFilenameInRpaths(path, dependent_file);
    }
    else if (realpath(path.c_str(), buffer)) {
        original_file = buffer;
    }
    else {
        warning_msg = "\n/!\\ WARNING: Cannot resolve path '" + path + "'\n";
        original_file = path;
    }

    if (db_->verboseOutput()) {
        std::cout<< "** Dependency ctor **" << std::endl;
        if (path != dependent_file)
            std::cout << "  dependent file:  " << dependent_file << std::endl;
        std::cout << "  dependency path: " << path << std::endl;
        std::cout << "  original_file:   " << original_file << std::endl;
    }

    // check if given path is a symlink
    if (original_file != path)
        AddSymlink(path);

    prefix = filePrefix(original_file);
    filename = stripPrefix(original_file);

    if (!prefix.empty() && prefix[prefix.size()-1] != '/')
        prefix += "/";

    // check if this dependency is in /usr/lib, /System/Library, or in ignored list
    if (!db_->isPrefixBundled(prefix))
        return;

    if (original_file.find(".framework") != std::string::npos) {
        is_framework = true;
        std::string framework_root = getFrameworkRoot(original_file);
        std::string framework_path = getFrameworkPath(original_file);
        std::string framework_name = stripPrefix(framework_root);
        prefix = filePrefix(framework_root);
        filename = framework_name + "/" + framework_path;
        if (db_->verboseOutput()) {
            std::cout << "  framework root: " << framework_root << std::endl;
            std::cout << "  framework path: " << framework_path << std::endl;
            std::cout << "  framework name: " << framework_name << std::endl;
        }
    }

    // check if the lib is in a known location
    if (prefix.empty() || !fileExists(prefix+filename)) {
        std::vector<std::string> search_paths = db_->searchPaths();
        if (search_paths.empty())
            db_->initSearchPaths();
        // check if file is contained in one of the paths
        for (const auto& search_path : search_paths) {
            if (fileExists(search_path+filename)) {
                warning_msg += "FOUND " + filename + " in " + search_path + "\n";
                prefix = search_path;
                db_->missingPrefixes(true);
                break;
            }
        }
    }

    if (!db_->quietOutput())
        std::cout << warning_msg;

    // if the location is still unknown, ask the user for search path
    if (!db_->isPrefixIgnored(prefix) && (prefix.empty() || !fileExists(prefix+filename))) {
        if (!db_->quietOutput())
            std::cerr << "\n/!\\ WARNING: Dependency " << filename << " of " << dependent_file << " not found\n";
        if (db_->verboseOutput())
            std::cout << "     path: " << (prefix+filename) << std::endl;
        db_->missingPrefixes(true);
        db_->addSearchPath(db_->getUserInputDirForFile(filename, dependent_file));
    }

    new_name = filename;
}

std::string Dependency::InnerPath() const
{
    return db_->insideLibPath() + new_name;
}

std::string Dependency::InstallPath() const
{
    return db_->destFolder() + new_name;
}

void Dependency::AddSymlink(const std::string& path)
{
    if (std::find(symlinks.begin(), symlinks.end(), path) == symlinks.end())
        symlinks.push_back(path);
}

bool Dependency::MergeIfIdentical(Dependency* dependency)
{
    if (dependency->OriginalFilename() == filename) {
        for (const auto& symlink : symlinks)
            dependency->AddSymlink(symlink);
        return true;
    }
    return false;
}

void Dependency::CopyToBundle() const
{
    std::string original_path = OriginalPath();
    std::string dest_path = InstallPath();

    if (is_framework) {
        original_path = getFrameworkRoot(original_path);
        dest_path = db_->destFolder() + stripPrefix(original_path);
    }

    if (db_->verboseOutput()) {
        std::string inner_path = InnerPath();
        std::cout << "  - original path: " << original_path << std::endl;
        std::cout << "  - inner path:    " << inner_path << std::endl;
        std::cout << "  - dest_path:     " << dest_path << std::endl;
        std::cout << "  - install path:  " << InstallPath() << std::endl;
    }

    db_->copyFile(original_path, dest_path);

    if (is_framework) {
        std::string headers_path = dest_path + std::string("/Headers");
        char buffer[PATH_MAX];
        if (realpath(rtrim(headers_path).c_str(), buffer))
            headers_path = buffer;
        db_->deleteFile(headers_path, true);
        db_->deleteFile(dest_path + "/*.prl");
    }

    db_->changeId(InstallPath(), "@rpath/" + new_name);
}

void Dependency::FixDependentFile(const std::string& dependent_file) const
{
    db_->changeInstallName(dependent_file, OriginalPath(), InnerPath());
    for (const auto& symlink : symlinks) {
        db_->changeInstallName(dependent_file, symlink, InnerPath());
    }

    if (db_->missingPrefixes()) {
        db_->changeInstallName(dependent_file, filename, InnerPath());
        for (const auto& symlink : symlinks) {
            db_->changeInstallName(dependent_file, symlink, InnerPath());
        }
    }
}

void Dependency::Print() const
{
    std::cout << "\n* " << filename << " from " << prefix << std::endl;
    for (const auto& symlink : symlinks) {
        std::cout << "    symlink --> " << symlink << std::endl;
    }
}
