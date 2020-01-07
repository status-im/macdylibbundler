#include "Settings.h"

#include <cstdlib>
#include <map>
#include <utility>

#include <sys/param.h>

#include "Utils.h"

Settings::Settings() : overwrite_files(false),
                       overwrite_dir(false),
                       create_dir(false),
                       quiet_output(false),
                       verbose_output(false),
                       bundle_libs(true),
                       bundle_frameworks(false),
                       missing_prefixes(false),
                       dest_folder_str("./libs/"),
                       dest_folder_str_app("./Frameworks/"),
                       inside_path_str("@executable_path/../libs/"),
                       inside_path_str_app("@executable_path/../Frameworks/")
{
    dest_folder = dest_folder_str;
    dest_path = dest_folder;
    inside_path = inside_path_str;
}

Settings::~Settings() = default;

void Settings::appBundle(std::string path)
{
    app_bundle = std::move(path);
    char buffer[PATH_MAX];
    if (realpath(app_bundle.c_str(), buffer))
        app_bundle = buffer;

    if (app_bundle[app_bundle.size()-1] != '/')
        app_bundle += "/"; // fix path if needed so it ends with '/'

    std::string bundle_executable_path = app_bundle + "Contents/MacOS/" + bundleExecutableName(app_bundle);
    if (realpath(bundle_executable_path.c_str(), buffer))
        bundle_executable_path = buffer;
    addFileToFix(bundle_executable_path);

    if (inside_path == inside_path_str)
        inside_path = inside_path_str_app;
    if (dest_folder == dest_folder_str)
        dest_folder = dest_folder_str_app;

    dest_path = app_bundle + "Contents/" + stripLSlash(dest_folder);
    if (realpath(dest_path.c_str(), buffer))
        dest_path = buffer;
    if (dest_path[dest_path.size()-1] != '/')
        dest_path += "/";
}

void Settings::destFolder(std::string path)
{
    dest_path = std::move(path);
    if (appBundleProvided())
        dest_path = app_bundle + "Contents/" + stripLSlash(dest_folder);
    char buffer[PATH_MAX];
    if (realpath(dest_path.c_str(), buffer))
        dest_path = buffer;
    if (dest_path[dest_path.size()-1] != '/')
        dest_path += "/";
}


void Settings::insideLibPath(std::string p)
{
    inside_path = std::move(p);
    if (inside_path[inside_path.size()-1] != '/')
        inside_path += "/";
}

void Settings::addFileToFix(std::string path)
{
    char buffer[PATH_MAX];
    if (realpath(path.c_str(), buffer))
        path = buffer;
    files.push_back(path);
}

void Settings::ignorePrefix(std::string prefix)
{
    if (prefix[prefix.size()-1] != '/')
        prefix += "/";
    prefixes_to_ignore.push_back(prefix);
}

bool Settings::isPrefixIgnored(const std::string& prefix)
{
    for (const auto& prefix_to_ignore : prefixes_to_ignore) {
        if (prefix == prefix_to_ignore)
            return true;
    }
    return false;
}

bool Settings::isPrefixBundled(const std::string& prefix)
{
    if (!bundle_frameworks && prefix.find(".framework") != std::string::npos)
        return false;
    if (prefix.find("@executable_path") != std::string::npos)
        return false;
    if (prefix.find("/usr/lib/") == 0)
        return false;
    if (prefix.find("/System/Library/") != std::string::npos)
        return false;
    if (isPrefixIgnored(prefix))
        return false;
    return true;
}
