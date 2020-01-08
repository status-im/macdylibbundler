#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#ifndef __clang__
#include <sys/types.h>
#endif

#include "DylibBundler.h"
#include "Settings.h"

const std::string VERSION = "2.1.0 (2020-01-04)";

void showHelp()
{
    std::cout << "Usage: dylibbundler -a file.app [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -a,  --app                   Application bundle to make self-contained" << std::endl;
    std::cout << "  -x,  --fix-file              Copy file's dependencies to app bundle and fix internal names and rpaths" << std::endl;
    std::cout << "  -f,  --frameworks            Copy dependencies that are frameworks (experimental)" << std::endl;
    std::cout << "  -d,  --dest-dir              Directory to copy dependencies, relative to <app>/Contents (default: ./Frameworks)" << std::endl;
    std::cout << "  -p,  --install-path          Inner path (@rpath) of bundled dependencies (default: @executable_path/../Frameworks/)" << std::endl;
    std::cout << "  -s,  --search-path           Add directory to search path" << std::endl;
    std::cout << "  -i,  --ignore                Ignore dependencies in this directory (default: /usr/lib & /System/Library)" << std::endl;
    std::cout << "  -of, --overwrite-files       Allow overwriting files in output directory" << std::endl;
    std::cout << "  -cd, --create-dir            Create output directory if needed" << std::endl;
    std::cout << "  -od, --overwrite-dir         Overwrite (delete) output directory if it exists (implies --create-dir)" << std::endl;
    std::cout << "  -n,  --just-print            Print the dependencies found (without copying into app bundle)" << std::endl;
    std::cout << "  -q,  --quiet                 Less verbose output" << std::endl;
    std::cout << "  -v,  --verbose               More verbose output" << std::endl;
    std::cout << "  -V,  --version               Print dylibbundler version number and exit" << std::endl;
    std::cout << "  -h,  --help                  Print this message and exit" << std::endl;
}

int main(int argc, const char* argv[])
{
    // parse arguments
    for (int i=0; i<argc; i++) {
        if (strcmp(argv[i],"-a") == 0 || strcmp(argv[i],"--app") == 0) {
            i++;
            Settings::appBundle(argv[i]);
            continue;
        }
        else if (strcmp(argv[i],"-x") == 0 || strcmp(argv[i],"--fix-file") == 0) {
            i++;
            Settings::addFileToFix(argv[i]);
            continue;
        }
        else if (strcmp(argv[i],"-f") == 0 || strcmp(argv[i],"--bundle-frameworks") == 0) {
            Settings::bundleFrameworks(true);
            continue;
        }
        else if (strcmp(argv[i],"-d") == 0 || strcmp(argv[i],"--dest-dir") == 0) {
            i++;
            Settings::destFolder(argv[i]);
            continue;
        }
        else if (strcmp(argv[i],"-p") == 0 || strcmp(argv[i],"--install-path") == 0) {
            i++;
            Settings::insideLibPath(argv[i]);
            continue;
        }
        else if (strcmp(argv[i],"-s") == 0 || strcmp(argv[i],"--search-path") == 0) {
            i++;
            Settings::addUserSearchPath(argv[i]);
            continue;
        }
        else if (strcmp(argv[i],"-i") == 0 || strcmp(argv[i],"--ignore") == 0) {
            i++;
            Settings::ignorePrefix(argv[i]);
            continue;
        }
        else if (strcmp(argv[i],"-of") == 0 || strcmp(argv[i],"--overwrite-files") == 0) {
            Settings::canOverwriteFiles(true);
            continue;
        }
        else if (strcmp(argv[i],"-cd") == 0 || strcmp(argv[i],"--create-dir") == 0) {
            Settings::canCreateDir(true);
            continue;
        }
        else if (strcmp(argv[i],"-od") == 0 || strcmp(argv[i],"--overwrite-dir") == 0) {
            Settings::canOverwriteDir(true);
            Settings::canCreateDir(true);
            continue;
        }
        else if (strcmp(argv[i],"-n") == 0 || strcmp(argv[i],"--just-print") == 0) {
            Settings::bundleLibs(false);
            continue;
        }
        else if (strcmp(argv[i],"-q") == 0 || strcmp(argv[i],"--quiet") == 0) {
            Settings::quietOutput(true);
            continue;
        }
        else if (strcmp(argv[i],"-v") == 0 || strcmp(argv[i],"--verbose") == 0) {
            Settings::verboseOutput(true);
            continue;
        }
        else if (strcmp(argv[i],"-b") == 0 || strcmp(argv[i],"--bundle-libs") == 0) {
            // old flag, on by default now. ignore.
            continue;
        }
        else if (strcmp(argv[i],"-V") == 0 || strcmp(argv[i],"--version") == 0) {
            std::cout << "dylibbundler " << VERSION << std::endl;
            exit(0);
        }
        else if (strcmp(argv[i],"-h") == 0 || strcmp(argv[i],"--help") == 0) {
            showHelp();
            exit(0);
        }
        else if (i > 0) {
            // unknown flag, abort
            std::cerr << "Unknown flag " << argv[i] << std::endl << std::endl;
            showHelp();
            exit(1);
        }
    }

    if (Settings::filesToFixCount() < 1) {
        showHelp();
        exit(0);
    }

    std::cout << "Collecting dependencies...\n";

    const std::vector<std::string> files_to_fix = Settings::filesToFix();
    for (const auto& file_to_fix : files_to_fix)
        collectDependenciesRpaths(file_to_fix);
    collectSubDependencies();
    bundleDependencies();

    return 0;
}
