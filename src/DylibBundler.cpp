#include "DylibBundler.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <numeric>
#include <regex>
#include <sstream>
#include <utility>

#ifdef __linux
#include <linux/limits.h>
#endif

#include "Dependency.h"
#include "Utils.h"

DylibBundler::DylibBundler() : qt_plugins_called(false) {}

DylibBundler::~DylibBundler() = default;

void DylibBundler::addDependency(const std::string& path, const std::string& dependent_file)
{
    Dependency* dependency = new Dependency(path, dependent_file, this);

    // check if this library was already added to |deps| to avoid duplicates
    bool in_deps = false;
    for (auto& dep : deps) {
        if (dependency->MergeIfIdentical(dep))
            in_deps = true;
    }

    // check if this library was already added to |deps_per_file[dependent_file]| to avoid duplicates
    bool in_deps_per_file = false;
    for (auto& dep : deps_per_file[dependent_file]) {
        if (dependency->MergeIfIdentical(dep))
            in_deps_per_file = true;
    }

    // check if this library is in /usr/lib, /System/Library, or in ignored list
    if (!isPrefixBundled(dependency->Prefix()))
        return;

    if (!in_deps && dependency->IsFramework())
        frameworks.insert(dependency->OriginalPath());
    if (!in_deps)
        deps.push_back(dependency);
    if (!in_deps_per_file)
        deps_per_file[dependent_file].push_back(dependency);
}

void DylibBundler::collectDependenciesRpaths(const std::string& dependent_file)
{
    if (deps_collected.find(dependent_file) != deps_collected.end() && fileHasRpath(dependent_file))
        return;

    std::map<std::string, std::string> cmds_values;
    std::string dylib = "LC_LOAD_DYLIB";
    std::string rpath = "LC_RPATH";
    cmds_values[dylib] = "name";
    cmds_values[rpath] = "path";
    std::map<std::string, std::vector<std::string>> cmds_results;

    parseLoadCommands(dependent_file, cmds_values, cmds_results);

    if (rpaths_collected.find(dependent_file) == rpaths_collected.end()) {
        auto rpath_results = cmds_results[rpath];
        for (const auto& rpath_result : rpath_results) {
            rpaths.insert(rpath_result);
            addRpathForFile(dependent_file, rpath_result);
            if (verboseOutput())
                std::cout << "  rpath: " << rpath_result << std::endl;
        }
        rpaths_collected[dependent_file] = true;
    }

    if (deps_collected.find(dependent_file) == deps_collected.end()) {
        auto dylib_results = cmds_results[dylib];
        for (const auto& dylib_result : dylib_results) {
            // skip system/ignored prefixes
            if (isPrefixBundled(dylib_result))
                addDependency(dylib_result, dependent_file);
        }
        deps_collected[dependent_file] = true;
    }
}

void DylibBundler::collectSubDependencies()
{
    if (verboseOutput()) {
        std::cout << "(pre sub) # OF FILES: " << filesToFixCount() << std::endl;
        std::cout << "(pre sub) # OF DEPS: " << deps.size() << std::endl;
    }
    size_t dep_counter = deps.size();
    size_t deps_size = deps.size();
    while (true) {
        deps_size = deps.size();
        for (size_t n=0; n<deps_size; ++n) {
            std::string original_path = deps[n]->OriginalPath();
            if (verboseOutput())
                std::cout << "  (collect sub deps) original path: " << original_path << std::endl;
            if (isRpath(original_path))
                original_path = searchFilenameInRpaths(original_path);

            collectDependenciesRpaths(original_path);
        }
        // if no more dependencies were added on this iteration, stop searching
        if (deps.size() == deps_size)
            break;
    }

    if (verboseOutput()) {
        std::cout << "(post sub) # OF FILES: " << filesToFixCount() << std::endl;
        std::cout << "(post sub) # OF DEPS: " << deps.size() << std::endl;
    }
    if (bundleLibs() && bundleFrameworks()) {
        if (!qt_plugins_called || (deps.size() != dep_counter))
            bundleQtPlugins();
    }
}

void DylibBundler::changeLibPathsOnFile(const std::string& file_to_fix)
{
    if (deps_collected.find(file_to_fix) == deps_collected.end() || rpaths_collected.find(file_to_fix) == rpaths_collected.end())
        collectDependenciesRpaths(file_to_fix);

    std::cout << "* Fixing dependencies on " << file_to_fix << "\n";
    std::vector<Dependency*> dependencies = deps_per_file[file_to_fix];
    for (auto& dependency : dependencies)
        dependency->FixDependentFile(file_to_fix);
}

void DylibBundler::fixRpathsOnFile(const std::string& original_file, const std::string& file_to_fix)
{
    std::vector<std::string> rpaths_to_fix;
    if (!fileHasRpath(original_file))
        return;

    rpaths_to_fix = getRpathsForFile(original_file);
    for (const auto& rpath_to_fix : rpaths_to_fix) {
        std::string command = std::string("install_name_tool -rpath ");
        command.append(rpath_to_fix).append(" ").append(insideLibPath());
        command.append(" ").append(file_to_fix);
        if (systemp(command) != 0) {
            std::cerr << "\n\n/!\\ ERROR: An error occured while trying to fix rpath " << rpath_to_fix << " of " << file_to_fix << std::endl;
            exit(1);
        }
    }
}

void DylibBundler::bundleDependencies()
{
    for (const auto& dep : deps)
        dep->Print();
    std::cout << "\n";
    if (verboseOutput()) {
        std::cout << "rpaths:" << std::endl;
        for (const auto& rpath : rpaths)
            std::cout << "* " << rpath << std::endl;
    }
    // copy & fix up dependencies
    if (bundleLibs()) {
        createDestDir();
        for (const auto& dep : deps) {
            dep->CopyToBundle();
            changeLibPathsOnFile(dep->InstallPath());
            fixRpathsOnFile(dep->OriginalPath(), dep->InstallPath());
        }
    }
    // fix up input files
    const auto files = filesToFix();
    for (const auto& file : files) {
        changeLibPathsOnFile(file);
        fixRpathsOnFile(file, file);
    }
}

void DylibBundler::bundleQtPlugins()
{
    bool qtCoreFound = false;
    bool qtGuiFound = false;
    bool qtNetworkFound = false;
    bool qtSqlFound = false;
    bool qtSvgFound = false;
    bool qtMultimediaFound = false;
    bool qt3dRenderFound = false;
    bool qt3dQuickRenderFound = false;
    bool qtPositioningFound = false;
    bool qtLocationFound = false;
    bool qtTextToSpeechFound = false;
    bool qtWebViewFound = false;
    std::string original_file;

    for (const auto& framework : frameworks) {
        if (framework.find("QtCore") != std::string::npos) {
            qtCoreFound = true;
            original_file = framework;
        }
        if (framework.find("QtGui") != std::string::npos)
            qtGuiFound = true;
        if (framework.find("QtNetwork") != std::string::npos)
            qtNetworkFound = true;
        if (framework.find("QtSql") != std::string::npos)
            qtSqlFound = true;
        if (framework.find("QtSvg") != std::string::npos)
            qtSvgFound = true;
        if (framework.find("QtMultimedia") != std::string::npos)
            qtMultimediaFound = true;
        if (framework.find("Qt3DRender") != std::string::npos)
            qt3dRenderFound = true;
        if (framework.find("Qt3DQuickRender") != std::string::npos)
            qt3dQuickRenderFound = true;
        if (framework.find("QtPositioning") != std::string::npos)
            qtPositioningFound = true;
        if (framework.find("QtLocation") != std::string::npos)
            qtLocationFound = true;
        if (framework.find("TextToSpeech") != std::string::npos)
            qtTextToSpeechFound = true;
        if (framework.find("WebView") != std::string::npos)
            qtWebViewFound = true;
    }

    if (!qtCoreFound)
        return;
    if (!qt_plugins_called)
        createQtConf(resourcesFolder());
    qt_plugins_called = true;

    const auto fixupPlugin = [&](const std::string& plugin) {
        std::string dest = pluginsFolder();
        std::string framework_root = getFrameworkRoot(original_file);
        std::string prefix = filePrefix(framework_root);
        std::string qt_prefix = filePrefix(prefix.substr(0, prefix.size()-1));
        std::string qt_plugins_prefix = qt_prefix + "plugins/";
        if (fileExists(qt_plugins_prefix + plugin)) {
            mkdir(dest + plugin);
            copyFile(qt_plugins_prefix + plugin, dest);
            std::vector<std::string> files = lsDir(dest + plugin + "/");
            for (const auto& file : files) {
                std::string file_to_fix = dest + plugin + "/" + file;
                std::string new_rpath = std::string("@rpath/") + plugin + "/" + file;
                addFileToFix(file_to_fix);
                collectDependenciesRpaths(file_to_fix);
                changeId(file_to_fix, new_rpath);
            }
        }
    };

    std::string framework_root = getFrameworkRoot(original_file);
    std::string prefix = filePrefix(framework_root);
    std::string qt_prefix = filePrefix(prefix.substr(0, prefix.size()-1));
    std::string qt_plugins_prefix = qt_prefix + "plugins/";

    std::string dest = pluginsFolder();
    mkdir(dest + "platforms");
    copyFile(qt_plugins_prefix + "platforms/libqcocoa.dylib", dest + "platforms");
    addFileToFix(dest + "platforms/libqcocoa.dylib");
    collectDependenciesRpaths(dest + "platforms/libqcocoa.dylib");

    fixupPlugin("printsupport");
    fixupPlugin("styles");
    fixupPlugin("imageformats");
    fixupPlugin("iconengines");
    if (!qtSvgFound)
        systemp(std::string("rm -f ") + dest + "imageformats/libqsvg.dylib");
    if (qtGuiFound) {
        fixupPlugin("platforminputcontexts");
        fixupPlugin("virtualkeyboard");
    }
    if (qtNetworkFound)
        fixupPlugin("bearer");
    if (qtSqlFound)
        fixupPlugin("sqldrivers");
    if (qtMultimediaFound) {
        fixupPlugin("mediaservice");
        fixupPlugin("audio");
    }
    if (qt3dRenderFound) {
        fixupPlugin("sceneparsers");
        fixupPlugin("geometryloaders");
    }
    if (qt3dQuickRenderFound)
        fixupPlugin("renderplugins");
    if (qtPositioningFound)
        fixupPlugin("position");
    if (qtLocationFound)
        fixupPlugin("geoservices");
    if (qtTextToSpeechFound)
        fixupPlugin("texttospeech");
    if (qtWebViewFound)
        fixupPlugin("webview");

    collectSubDependencies();
}

std::string DylibBundler::getUserInputDirForFile(const std::string& filename, const std::string& dependent_file)
{
    std::vector<std::string> search_paths = userSearchPaths();
    for (auto& search_path : search_paths) {
        if (!search_path.empty() && search_path[search_path.size()-1] != '/')
            search_path += "/";
        if (fileExists(search_path + filename)) {
            if (!quietOutput()) {
                std::cerr << (search_path + filename) << " was found\n"
                          << "/!\\ WARNING: dylibbundler MAY NOT CORRECTLY HANDLE THIS DEPENDENCY: Check the executable with 'otool -L'\n";
            }
            return search_path;
        }
    }

    while (true) {
        if (quietOutput())
            std::cerr << "\n/!\\ WARNING: Dependency " << filename << " of " << dependent_file << " not found\n";
        std::cout << "\nPlease specify the directory where this file is located (or enter 'quit' to abort): ";
        fflush(stdout);

        std::string prefix;
        std::cin >> prefix;
        std::cout << std::endl;

        if (prefix == "quit" || prefix == "exit" || prefix == "abort")
            exit(1);
        if (!prefix.empty() && prefix[prefix.size()-1] != '/')
            prefix += "/";
        if (!fileExists(prefix+filename)) {
            std::cerr << (prefix+filename) << " does not exist. Try again...\n";
            continue;
        }
        else {
            std::cerr << (prefix+filename) << " was found\n"
                      << "/!\\ WARNING: dylibbundler MAY NOT CORRECTLY HANDLE THIS DEPENDENCY: Check the executable with 'otool -L'\n";
            addUserSearchPath(prefix);
            return prefix;
        }
    }
}

std::string DylibBundler::searchFilenameInRpaths(const std::string& rpath_file, const std::string& dependent_file)
{
    if (verboseOutput()) {
        if (dependent_file != rpath_file)
            std::cout << "  dependent file: " << dependent_file << std::endl;
        std::cout << "    dependency: " << rpath_file << std::endl;
    }
    std::string fullpath;
    std::string suffix = rpath_file.substr(rpath_file.rfind('/')+1);
    char fullpath_buffer[PATH_MAX];

    const auto check_path = [&](std::string path) {
        char buffer[PATH_MAX];
        std::string file_prefix = filePrefix(dependent_file);
        if (path.find("@executable_path") != std::string::npos || path.find("@loader_path") != std::string::npos) {
            if (path.find("@executable_path") != std::string::npos) {
                if (appBundleProvided())
                    path = std::regex_replace(path, std::regex("@executable_path/"), executableFolder());
            }
            if (dependent_file != rpath_file) {
                if (path.find("@loader_path") != std::string::npos)
                    path = std::regex_replace(path, std::regex("@loader_path/"), file_prefix);
            }
            if (verboseOutput())
                std::cout << "    path to search: " << path << std::endl;
            if (realpath(path.c_str(), buffer)) {
                fullpath = buffer;
                rpathToFullPath(rpath_file, fullpath);
                return true;
            }
        }
        else if (path.find("@rpath") != std::string::npos) {
            if (appBundleProvided()) {
                std::string pathE = std::regex_replace(path, std::regex("@rpath/"), executableFolder());
                if (verboseOutput())
                    std::cout << "    path to search: " << pathE << std::endl;
                if (realpath(pathE.c_str(), buffer)) {
                    fullpath = buffer;
                    rpathToFullPath(rpath_file, fullpath);
                    return true;
                }
            }
            if (dependent_file != rpath_file) {
                std::string pathL = std::regex_replace(path, std::regex("@rpath/"), file_prefix);
                if (verboseOutput())
                    std::cout << "    path to search: " << pathL << std::endl;
                if (realpath(pathL.c_str(), buffer)) {
                    fullpath = buffer;
                    rpathToFullPath(rpath_file, fullpath);
                    return true;
                }
            }
        }
        return false;
    };

    // fullpath previously stored
    if (rpathFound(rpath_file)) {
        fullpath = getFullPath(rpath_file);
    }
    else if (!check_path(rpath_file)) {
        auto rpaths_for_file = getRpathsForFile(dependent_file);
        for (auto rpath : rpaths_for_file) {
            if (rpath[rpath.size()-1] != '/')
                rpath += "/";
            std::string path = rpath + suffix;
            if (verboseOutput())
                std::cout << "    trying rpath: " << path << std::endl;
            if (check_path(path))
                break;
        }
    }

    if (fullpath.empty()) {
        std::vector<std::string> search_paths = searchPaths();
        for (const auto& search_path : search_paths) {
            if (fileExists(search_path+suffix)) {
                if (verboseOutput())
                    std::cout << "FOUND " << suffix << " in " << search_path << std::endl;
                fullpath = search_path + suffix;
                break;
            }
        }
        if (fullpath.empty()) {
            if (verboseOutput())
                std::cout << "  ** rpath fullpath: not found" << std::endl;
            if (!quietOutput())
                std::cerr << "\n/!\\ WARNING: Can't get path for '" << rpath_file << "'\n";
            fullpath = getUserInputDirForFile(suffix, dependent_file) + suffix;
            if (quietOutput() && fullpath.empty())
                std::cerr << "\n/!\\ WARNING: Can't get path for '" << rpath_file << "'\n";
            if (realpath(fullpath.c_str(), fullpath_buffer))
                fullpath = fullpath_buffer;
        }
        else if (verboseOutput()) {
            std::cout << "  ** rpath fullpath: " << fullpath << std::endl;
        }
    }
    else if (verboseOutput()) {
        std::cout << "  ** rpath fullpath: " << fullpath << std::endl;
    }
    return fullpath;
}

std::string DylibBundler::searchFilenameInRpaths(const std::string& rpath_file)
{
    return searchFilenameInRpaths(rpath_file, rpath_file);
}

void DylibBundler::initSearchPaths()
{
    std::string searchPaths;
    char *dyldLibPath = std::getenv("DYLD_LIBRARY_PATH");
    if (dyldLibPath != nullptr)
        searchPaths = dyldLibPath;
    dyldLibPath = std::getenv("DYLD_FALLBACK_FRAMEWORK_PATH");
    if (dyldLibPath != nullptr) {
        if (!searchPaths.empty() && searchPaths[searchPaths.size()-1] != ':')
            searchPaths += ":";
        searchPaths += dyldLibPath;
    }
    dyldLibPath = std::getenv("DYLD_FALLBACK_LIBRARY_PATH");
    if (dyldLibPath != nullptr) {
        if (!searchPaths.empty() && searchPaths[searchPaths.size()-1] != ':')
            searchPaths += ":";
        searchPaths += dyldLibPath;
    }
    if (!searchPaths.empty()) {
        std::stringstream ss(searchPaths);
        std::string item;
        while (std::getline(ss, item, ':')) {
            if (item[item.size()-1] != '/')
                item += "/";
            addSearchPath(item);
        }
    }
}

void DylibBundler::createDestDir()
{
    std::string dest_folder = destFolder();
    if (verboseOutput())
        std::cout << "Checking output directory " << dest_folder << "\n";

    bool dest_exists = fileExists(dest_folder);
    if (dest_exists && canOverwriteDir()) {
        std::cout << "Erasing old output directory " << dest_folder << "\n";
        std::string command = std::string("rm -r ").append(dest_folder);
        if (systemp(command) != 0) {
            std::cerr << "\n\n/!\\ ERROR: An error occured while attempting to overwrite destination folder\n";
            exit(1);
        }
        dest_exists = false;
    }

    if (!dest_exists) {
        if (canCreateDir()) {
            std::cout << "Creating output directory " << dest_folder << "\n\n";
            if (!mkdir(dest_folder)) {
                std::cerr << "\n/!\\ ERROR: An error occured while creating " << dest_folder << std::endl;
                exit(1);
            }
        }
        else {
            std::cerr << "\n\n/!\\ ERROR: Destination folder does not exist. Create it or pass the '-cd' or '-od' flag\n";
            exit(1);
        }
    }
}

void DylibBundler::changeId(const std::string& binary_file, const std::string& new_id)
{
    std::string command = std::string("install_name_tool -id ").append(new_id).append(" ").append(binary_file);
    if (systemp(command) != 0) {
        std::cerr << "\n\nError: An error occured while trying to change identity of library " << binary_file << std::endl;
        exit(1);
    }
}

void DylibBundler::changeInstallName(const std::string& binary_file, const std::string& old_name, const std::string& new_name)
{
    std::string command = std::string("install_name_tool -change ").append(old_name).append(" ");
    command.append(new_name).append(" ").append(binary_file);
    if (systemp(command) != 0) {
        std::cerr << "\n\nError: An error occured while trying to fix dependencies of " << binary_file << std::endl;
        exit(1);
    }
}

void DylibBundler::copyFile(const std::string& from, const std::string& to)
{
    bool overwrite = canOverwriteFiles();
    if (fileExists(to) && !overwrite) {
        std::cerr << "\n\nError: File " << to << " already exists. Remove it or enable overwriting (-of)\n";
        exit(1);
    }

    // copy file/directory
    std::string overwrite_permission = std::string(overwrite ? "-f " : "-n ");
    std::string command = std::string("cp -R ").append(overwrite_permission);
    command.append(from).append(" ").append(to);
    if (from != to && systemp(command) != 0) {
        std::cerr << "\n\nError: An error occured while trying to copy file " << from << " to " << to << std::endl;
        exit(1);
    }

    // give file/directory write permission
    std::string command2 = std::string("chmod -R +w ").append(to);
    if (systemp(command2) != 0) {
        std::cerr << "\n\nError: An error occured while trying to set write permissions on file " << to << std::endl;
        exit(1);
    }
}

void DylibBundler::deleteFile(const std::string& path, bool overwrite)
{
    std::string overwrite_permission = std::string(overwrite ? "-f " : " ");
    std::string command = std::string("rm -r ").append(overwrite_permission).append(path);
    if (systemp(command) != 0) {
        std::cerr << "\n\nError: An error occured while trying to delete " << path << std::endl;
        exit(1);
    }
}

void DylibBundler::deleteFile(const std::string& path)
{
    bool overwrite = canOverwriteFiles();
    deleteFile(path, overwrite);
}

bool DylibBundler::mkdir(const std::string& path)
{
    if (verboseOutput())
        std::cout << "Creating directory " << path << std::endl;
    std::string command = std::string("mkdir -p ").append(path);
    if (systemp(command) != 0) {
        std::cerr << "\n/!\\ ERROR: An error occured while creating " << path << std::endl;
        return false;
    }
    return true;
}

int DylibBundler::systemp(const std::string& cmd)
{
    if (!quietOutput())
        std::cout << "    " << cmd << "\n";
    return system(cmd.c_str());
}
