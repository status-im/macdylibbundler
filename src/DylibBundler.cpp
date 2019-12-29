#include "DylibBundler.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <regex>
#include <set>
#ifdef __linux
#include <linux/limits.h>
#endif

#include "Dependency.h"
#include "Settings.h"
#include "Utils.h"

std::vector<Dependency> deps;
std::map<std::string, std::vector<Dependency>> deps_per_file;
std::map<std::string, bool> deps_collected;
std::set<std::string> frameworks;
std::set<std::string> rpaths;
std::map<std::string, std::vector<std::string>> rpaths_per_file;
std::map<std::string, std::string> rpath_to_fullpath;
bool qt_plugins_called = false;

void initRpaths()
{
    std::string executable_path = Settings::executableFolder();
    std::string frameworks_path = Settings::frameworksFolder();
    std::string plugins_path = Settings::pluginsFolder();
    rpaths.insert(executable_path);
    rpaths.insert(frameworks_path);
    rpaths.insert(plugins_path);
}

void changeLibPathsOnFile(std::string file_to_fix)
{
    if (deps_collected.find(file_to_fix) == deps_collected.end())
        collectDependencies(file_to_fix);

    std::cout << "* Fixing dependencies on " << file_to_fix << "\n";

    const int dep_amount = deps_per_file[file_to_fix].size();
    for (size_t n=0; n<dep_amount; ++n) {
        deps_per_file[file_to_fix][n].fixFileThatDependsOnMe(file_to_fix);
    }
}

void collectRpaths(const std::string& filename)
{
    if (!fileExists(filename)) {
        std::cerr << "\n/!\\ WARNING: Can't collect rpaths for nonexistent file '" << filename << "'\n";
        return;
    }
    if (Settings::verboseOutput())
        std::cout << "  collecting rpaths for: " << filename << std::endl;

    size_t pos = 0;
    bool read_rpath = false;

    std::string cmd = "otool -l " + filename;
    std::string output = systemOutput(cmd);
    std::vector<std::string> lc_lines;

    tokenize(output, "\n", &lc_lines);

    while (pos < lc_lines.size()) {
        std::string line = lc_lines[pos];
        pos++;
        if (read_rpath) {
            size_t start_pos = line.find("path ");
            size_t end_pos = line.find(" (");
            if (start_pos == std::string::npos || end_pos == std::string::npos) {
                std::cerr << "\n/!\\ WARNING: Unexpected LC_RPATH format\n";
                continue;
            }
            start_pos += 5; // to exclude "path "
            std::string rpath = line.substr(start_pos, end_pos - start_pos);
            rpaths.insert(rpath);
            rpaths_per_file[filename].push_back(rpath);
            read_rpath = false;
            if (Settings::verboseOutput())
                std::cout << "  rpath: " << rpath << std::endl;
            continue;
        }
        if (line.find("LC_RPATH") != std::string::npos) {
            read_rpath = true;
            pos++;
        }
    }
}

void collectRpathsForFilename(const std::string& filename)
{
    if (rpaths_per_file.find(filename) == rpaths_per_file.end())
        collectRpaths(filename);
}

std::string searchFilenameInRpaths(const std::string& rpath_file, const std::string& dependent_file)
{
    if (Settings::verboseOutput()) {
        if (dependent_file != rpath_file)
            std::cout << "  dependent file: " << dependent_file << std::endl;
        std::cout << "    dependency: " << rpath_file << std::endl;
    }

    std::string fullpath;
    std::string suffix = rpath_file.substr(rpath_file.rfind("/")+1);
    char fullpath_buffer[PATH_MAX];

    auto check_path = [&](std::string path) {
        char buffer[PATH_MAX];
        std::string file_prefix = filePrefix(dependent_file);
        if (path.find("@executable_path") != std::string::npos || path.find("@loader_path") != std::string::npos) {
            if (path.find("@executable_path") != std::string::npos)
                path = std::regex_replace(path, std::regex("@executable_path/"), Settings::executableFolder());
            if (dependent_file != rpath_file) {
                if (path.find("@loader_path") != std::string::npos)
                    path = std::regex_replace(path, std::regex("@loader_path/"), file_prefix);
            }
            if (Settings::verboseOutput())
                std::cout << "    path to search: " << path << std::endl;
            if (realpath(path.c_str(), buffer)) {
                fullpath = buffer;
                rpath_to_fullpath[rpath_file] = fullpath;
                return true;
            }
        }
        else if (path.find("@rpath") != std::string::npos) {
            std::string pathE = std::regex_replace(path, std::regex("@rpath/"), Settings::executableFolder());
            std::string pathL = std::regex_replace(path, std::regex("@rpath/"), file_prefix);
            if (Settings::verboseOutput())
                std::cout << "    path to search: " << pathE << std::endl;
            if (realpath(pathE.c_str(), buffer)) {
                fullpath = buffer;
                rpath_to_fullpath[rpath_file] = fullpath;
                return true;
            }
            if (dependent_file != rpath_file) {
                if (Settings::verboseOutput())
                    std::cout << "    path to search: " << pathL << std::endl;
                if (realpath(pathL.c_str(), buffer)) {
                    fullpath = buffer;
                    rpath_to_fullpath[rpath_file] = fullpath;
                    return true;
                }
            }
        }
        return false;
    };

    // fullpath previously stored
    if (rpath_to_fullpath.find(rpath_file) != rpath_to_fullpath.end()) {
        fullpath = rpath_to_fullpath[rpath_file];
    }
    else {
        if (!check_path(rpath_file)) {
            for (auto it = rpaths_per_file[dependent_file].begin(); it != rpaths_per_file[dependent_file].end(); ++it) {
                std::string rpath = *it;
                if (rpath[rpath.size()-1] != '/')
                    rpath += "/";
                std::string path = rpath + suffix;
                if (Settings::verboseOutput())
                    std::cout << "    trying rpath: " << path << std::endl;
                if (check_path(path))
                    break;
            }
        }
    }

    if (Settings::verboseOutput()) {
        if (!fullpath.empty()) {
            std::cout << "  ** rpath fullpath: " << fullpath << std::endl;
        }
        else {
            std::cout << "  ** rpath fullpath: not found" << std::endl;
        }
    }

    if (fullpath.empty()) {
        if (!Settings::quietOutput())
            std::cerr << "\n/!\\ WARNING: Can't get path for '" << rpath_file << "'\n";
        fullpath = getUserInputDirForFile(suffix) + suffix;
        if (Settings::quietOutput() && fullpath.empty())
            std::cerr << "\n/!\\ WARNING: Can't get path for '" << rpath_file << "'\n";
        if (realpath(fullpath.c_str(), fullpath_buffer))
            fullpath = fullpath_buffer;
    }

    return fullpath;
}

std::string searchFilenameInRpaths(const std::string& rpath_file)
{
    return searchFilenameInRpaths(rpath_file, rpath_file);
}

void fixRpathsOnFile(const std::string& original_file, const std::string& file_to_fix)
{
    std::vector<std::string> rpaths_to_fix;
    std::map<std::string, std::vector<std::string>>::iterator found = rpaths_per_file.find(original_file);
    if (found != rpaths_per_file.end())
        rpaths_to_fix = found->second;

    for (size_t i=0; i < rpaths_to_fix.size(); ++i) {
        std::string command =
            std::string("install_name_tool -rpath ")
                + rpaths_to_fix[i] + " "
                + Settings::insideLibPath() + " "
                + file_to_fix;
        if (systemp(command) != 0) {
            std::cerr << "\n\n/!\\ ERROR: An error occured while trying to fix dependencies of " << file_to_fix << "\n";
            exit(1);
        }
    }
}

void addDependency(std::string path, std::string dependent_file)
{
    Dependency dep(path, dependent_file);

    // check if this library was already added to |deps| to avoid duplicates
    bool in_deps = false;
    for (size_t n=0; n<deps.size(); ++n) {
        if (dep.mergeIfSameAs(deps[n]))
            in_deps = true;
    }

    // check if this library was already added to |deps_per_file[dependent_file]| to avoid duplicates
    bool in_deps_per_file = false;
    for (size_t n=0; n<deps_per_file[dependent_file].size(); ++n) {
        if (dep.mergeIfSameAs(deps_per_file[dependent_file][n]))
            in_deps_per_file = true;
    }

    // check if this library is in /usr/lib, /System/Library, or in ignored list
    if (!Settings::isPrefixBundled(dep.getPrefix()))
        return;

    if (!in_deps && dep.isFramework())
        frameworks.insert(dep.getOriginalPath());

    if (!in_deps)
        deps.push_back(dep);

    if (!in_deps_per_file)
        deps_per_file[dependent_file].push_back(dep);
}

// Fill |lines| with dependencies of given |filename|
void collectDependencies(std::string dependent_file, std::vector<std::string>& lines)
{
    std::string cmd = "otool -l " + dependent_file;
    std::string output = systemOutput(cmd);

    if (output.find("can't open file") != std::string::npos
            || output.find("No such file") != std::string::npos
            || output.size() < 1) {
        std::cerr << "\n\n/!\\ ERROR: Cannot find file " << dependent_file << " to read its dependencies\n";
        exit(1);
    }

    std::vector<std::string> raw_lines;
    tokenize(output, "\n", &raw_lines);

    bool searching = false;
    for (const auto& line : raw_lines) {
        if (line.find("cmd LC_LOAD_DYLIB") != std::string::npos) {
            if (searching) {
                std::cerr << "\n\n/!\\ ERROR: Failed to find name before next cmd" << std::endl;
                exit(1);
            }
            searching = true;
        }
        else if (searching) {
            size_t found = line.find("name ");
            if (found != std::string::npos) {
                lines.push_back('\t' + line.substr(found+5, std::string::npos));
                searching = false;
            }
        }
    }
}

void collectDependencies(std::string dependent_file)
{
    std::vector<std::string> lines;
    collectDependencies(dependent_file, lines);

    for (size_t n=0; n<lines.size(); n++) {
        if (!Settings::bundleFrameworks()) {
            if (lines[n].find(".framework") != std::string::npos)
                continue;
        }
        // lines containing path begin with a tab
        if (lines[n][0] != '\t')
            continue;
        // trim useless info, keep only library path
        std::string dep_path = lines[n].substr(1, lines[n].rfind(" (") - 1);
        if (isRpath(dep_path))
            collectRpathsForFilename(dependent_file);

        addDependency(dep_path, dependent_file);
    }
    deps_collected[dependent_file] = true;
}

// recursively collect each dependency's dependencies
void collectSubDependencies()
{
    size_t dep_counter = deps.size();
    if (Settings::verboseOutput()) {
        std::cout << "(pre sub) # OF FILES: " << Settings::filesToFixCount() << std::endl;
        std::cout << "(pre sub) # OF DEPS: " << deps.size() << std::endl;
    }

    size_t deps_size = deps.size();
    while (true) {
        deps_size = deps.size();

        for (size_t n=0; n<deps_size; n++) {
            std::string original_path = deps[n].getOriginalPath();
            if (Settings::verboseOutput())
                std::cout << "  (collect sub deps) original path: " << original_path << std::endl;
            if (isRpath(original_path))
                original_path = searchFilenameInRpaths(original_path);

            collectRpathsForFilename(original_path);

            std::vector<std::string> lines;
            collectDependencies(original_path, lines);

            for (size_t n=0; n<lines.size(); ++n) {
                if (!Settings::bundleFrameworks()) {
                    if (lines[n].find(".framework") != std::string::npos)
                        continue;
                }
                // lines containing path begin with a tab
                if (lines[n][0] != '\t')
                    continue;
                // trim useless info, keep only library name
                std::string dep_path = lines[n].substr(1, lines[n].rfind(" (") - 1);
                if (isRpath(dep_path)) {
                    std::string full_path = searchFilenameInRpaths(dep_path, original_path);
                    collectRpathsForFilename(full_path);
                }
                addDependency(dep_path, original_path);
            }
        }
        // if no more dependencies were added on this iteration, stop searching
        if (deps.size() == deps_size) {
            break;
        }
    }
    if (Settings::verboseOutput()) {
        std::cout << "(post sub) # OF FILES: " << Settings::filesToFixCount() << std::endl;
        std::cout << "(post sub) # OF DEPS: " << deps.size() << std::endl;
    }
    if (Settings::bundleFrameworks()) {
        if (!qt_plugins_called || (deps.size() != dep_counter))
            copyQtPlugins();
    }
}

void doneWithDeps_go()
{
    if (Settings::verboseOutput()) {
        for (std::set<std::string>::iterator it = rpaths.begin(); it != rpaths.end(); ++it) {
            std::cout << "rpaths: " << *it << std::endl;
        }
    }

    const size_t deps_size = deps.size();

    for (size_t n=0; n<deps_size; ++n)
        deps[n].print();
    std::cout << "\n";

    // copy files if requested by user
    if (Settings::bundleLibs()) {
        createDestDir();
        for (size_t i=0; i<deps_size; ++i) {
            deps[i].copyYourself();
            changeLibPathsOnFile(deps[i].getInstallPath());
            fixRpathsOnFile(deps[i].getOriginalPath(), deps[i].getInstallPath());
        }
    }

    const size_t filesToFixSize = Settings::filesToFix().size();
    for (size_t n=0; n<filesToFixSize; ++n) {
        changeLibPathsOnFile(Settings::fileToFix(n));
        fixRpathsOnFile(Settings::fileToFix(n), Settings::fileToFix(n));
    }
}

void createQtConf(std::string directory)
{
    std::string contents = "[Paths]\n"
                           "Plugins = PlugIns\n"
                           "Imports = Resources/qml\n"
                           "Qml2Imports = Resources/qml\n";
    if (directory[directory.size()-1] != '/')
        directory += "/";
    std::ofstream out(directory + "qt.conf");
    out << contents;
    out.close();
}

void copyQtPlugins()
{
    qt_plugins_called = true;
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

    for (std::set<std::string>::iterator it = frameworks.begin(); it != frameworks.end(); ++it) {
        std::string framework = *it;
        if (framework.find("QtCore") != std::string::npos) {
            qtCoreFound = true;
            original_file = framework;
        }
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

    createQtConf(Settings::resourcesFolder());

    std::string dest = Settings::pluginsFolder();

    auto fixupPlugin = [original_file,dest](std::string plugin) {
        std::string framework_root = getFrameworkRoot(original_file);
        std::string prefix = filePrefix(framework_root);
        std::string qt_prefix = filePrefix(prefix.substr(0, prefix.size()-1));
        std::string qt_plugins_prefix = qt_prefix + "plugins/";
        if (fileExists(qt_plugins_prefix + plugin)) {
            mkdir(dest + plugin);
            copyFile(qt_plugins_prefix + plugin, dest);
            std::vector<std::string> files = lsDir(dest + plugin+"/");
            for (const auto& file : files) {
                Settings::addFileToFix(dest + plugin+"/"+file);
                collectDependencies(dest + plugin+"/"+file);
                changeId(dest + plugin+"/"+file, "@rpath/" + plugin+"/"+file);
            }
        }
    };

    std::string framework_root = getFrameworkRoot(original_file);
    std::string prefix = filePrefix(framework_root);
    std::string qt_prefix = filePrefix(prefix.substr(0, prefix.size()-1));
    std::string qt_plugins_prefix = qt_prefix + "plugins/";

    mkdir(dest + "platforms");
    copyFile(qt_plugins_prefix + "platforms/libqcocoa.dylib", dest + "platforms");
    Settings::addFileToFix(dest + "platforms/libqcocoa.dylib");
    collectDependencies(dest + "platforms/libqcocoa.dylib");

    fixupPlugin("printsupport");
    fixupPlugin("styles");
    fixupPlugin("imageformats");
    fixupPlugin("iconengines");
    if (!qtSvgFound)
        systemp("rm -f " + dest + "imageformats/libqsvg.dylib");
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
