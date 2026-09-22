// See respath.h.

#include "respath.h"

#include <sys/stat.h>
#include <unistd.h>

#include <cstdlib>
#include <string>

#ifndef JACKBRIDGE_RESOURCE_DIR_DEFAULT
#error "JACKBRIDGE_RESOURCE_DIR_DEFAULT must be defined by the build (the installed share dir)"
#endif

namespace jackbridge
{
namespace
{

bool hasFonts(const std::string &dir)
{
    if (dir.empty())
        return false;
    struct stat st;
    return stat((dir + "/fonts").c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

// The directory the running executable sits in, or empty.
std::string exeDir()
{
    char buf[4096];
    const ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0)
        return std::string();
    buf[n] = '\0';
    std::string path(buf);
    const size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? std::string() : path.substr(0, slash);
}

std::string resolve()
{
    if (const char *env = getenv("JACKBRIDGE_RESOURCE_DIR")) {
        const std::string dir(env);
        if (hasFonts(dir))
            return dir;
    }

    const std::string installed(JACKBRIDGE_RESOURCE_DIR_DEFAULT);
    if (hasFonts(installed))
        return installed;

    const std::string exe = exeDir();
    if (!exe.empty()) {
        // A build tree. Both binaries land in contrib/bin/, so resources/ is two levels up;
        // "/resources" covers an uninstalled binary run from the repository root. An uninstalled
        // build that cannot find the fonts falls back to a system face, and FontStack reports it
        // -- see fontstack.h. That is a warning and not a failure, but it means the layout the
        // audit passed is not the layout on screen, so it is worth finding here.
        for (const char *rel : {"/resources", "/../resources", "/../../resources"}) {
            const std::string dir = exe + rel;
            if (hasFonts(dir))
                return dir;
        }
    }

    return std::string();
}

} // namespace

const std::string &resourceDir()
{
    static const std::string dir = resolve();
    return dir;
}

} // namespace jackbridge
