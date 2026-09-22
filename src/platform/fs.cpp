// See fs.h.

#include "fs.h"

#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace jackbridge
{
namespace fs
{
namespace
{

std::string dirNameOf(const std::string &path)
{
    const size_t slash = path.find_last_of('/');
    if (slash == std::string::npos)
        return ".";
    if (slash == 0)
        return "/";
    return path.substr(0, slash);
}

std::string envOr(const char *name, const std::string &fallback)
{
    const char *v = getenv(name);
    return (v && *v) ? std::string(v) : fallback;
}

} // namespace

bool exists(const std::string &path)
{
    struct stat st;
    return !path.empty() && stat(path.c_str(), &st) == 0;
}

bool readFile(const std::string &path, std::string &out)
{
    FILE *f = fopen(path.c_str(), "rb");
    if (!f)
        return false;
    out.clear();
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        out.append(buf, n);
    const bool ok = ferror(f) == 0;
    fclose(f);
    return ok;
}

bool makeDirs(const std::string &path)
{
    if (path.empty())
        return false;
    struct stat st;
    if (stat(path.c_str(), &st) == 0)
        return S_ISDIR(st.st_mode);

    const std::string parent = dirNameOf(path);
    if (parent != path && parent != "/" && parent != "." && !makeDirs(parent))
        return false;

    // 0700: everything this program writes under the config and runtime directories is the
    // user's own, and the runtime files name a pid this user is about to signal.
    if (mkdir(path.c_str(), 0700) == 0)
        return true;
    return errno == EEXIST && stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool writeFileAtomic(const std::string &path, const std::string &body)
{
    const std::string dir = dirNameOf(path);
    if (!makeDirs(dir))
        return false;

    // The temporary must be in the SAME directory as the target: rename() is only atomic within
    // one filesystem, and /tmp is very often a different one from $HOME.
    std::string tmp = path + ".tmp-XXXXXX";
    std::vector<char> tpl(tmp.begin(), tmp.end());
    tpl.push_back('\0');

    const int fd = mkstemp(tpl.data());
    if (fd < 0)
        return false;
    tmp.assign(tpl.data());

    bool ok = true;
    size_t written = 0;
    while (ok && written < body.size()) {
        const ssize_t n = write(fd, body.data() + written, body.size() - written);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            ok = false;
            break;
        }
        written += static_cast<size_t>(n);
    }

    // fsync before the rename, not after: the rename can otherwise be durable while the contents
    // it points at are not, which is the crash that leaves a zero-length ~/.asoundrc behind.
    if (ok && fsync(fd) != 0)
        ok = false;
    if (close(fd) != 0)
        ok = false;

    // mkstemp makes the file 0600; these are ordinary config files and should be readable the
    // way the user's umask says the rest of their config is.
    if (ok && chmod(tmp.c_str(), 0644) != 0)
        ok = false;

    if (ok && rename(tmp.c_str(), path.c_str()) != 0)
        ok = false;

    if (!ok)
        unlink(tmp.c_str());
    return ok;
}

bool removeFile(const std::string &path)
{
    return unlink(path.c_str()) == 0 || errno == ENOENT;
}

const std::string &homeDir()
{
    static const std::string dir = [] {
        const char *h = getenv("HOME");
        if (h && *h)
            return std::string(h);
        if (const struct passwd *pw = getpwuid(getuid()))
            if (pw->pw_dir && *pw->pw_dir)
                return std::string(pw->pw_dir);
        return std::string("/");
    }();
    return dir;
}

const std::string &configHome()
{
    static const std::string dir = envOr("XDG_CONFIG_HOME", homeDir() + "/.config");
    return dir;
}

const std::string &runtimeDir()
{
    static const std::string dir = envOr("XDG_RUNTIME_DIR", "/tmp");
    return dir;
}

namespace
{

// One line of user-dirs.dirs: KEY="value" or KEY=value, with a leading $HOME or ${HOME} expanded.
// Comments start with #. This is the subset xdg-user-dirs actually writes -- it generates the file
// itself and always quotes -- so the parser matches the generator rather than the whole of shell
// quoting, which the file's own format is not.
std::string parseUserDir(const std::string &body, const char *key)
{
    const std::string want(key);
    size_t pos = 0;
    while (pos < body.size()) {
        size_t eol = body.find('\n', pos);
        if (eol == std::string::npos)
            eol = body.size();
        std::string line = body.substr(pos, eol - pos);
        pos = eol + 1;

        // Leading whitespace, then a comment or the key.
        size_t i = line.find_first_not_of(" \t");
        if (i == std::string::npos || line[i] == '#')
            continue;
        if (line.compare(i, want.size(), want) != 0)
            continue;
        size_t eq = line.find('=', i + want.size());
        if (eq == std::string::npos)
            continue;
        // Only the exact key, so XDG_MUSIC_DIR_OLD does not answer for XDG_MUSIC_DIR.
        if (line.find_first_not_of(" \t", i + want.size()) != eq)
            continue;

        std::string value = line.substr(eq + 1);
        size_t b = value.find_first_not_of(" \t");
        if (b == std::string::npos)
            continue;
        size_t e = value.find_last_not_of(" \t\r");
        value = value.substr(b, e - b + 1);
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.size() - 2);
        if (value.empty())
            continue;

        for (const char *prefix : {"$HOME", "${HOME}"}) {
            const size_t n = strlen(prefix);
            if (value.compare(0, n, prefix) == 0) {
                value = homeDir() + value.substr(n);
                break;
            }
        }
        // A relative path here would be relative to nothing in particular; treat it as unset.
        if (value.empty() || value[0] != '/')
            continue;
        return value;
    }
    return std::string();
}

} // namespace

const std::string &musicDir()
{
    static const std::string dir = [] {
        std::string body;
        if (readFile(configHome() + "/user-dirs.dirs", body)) {
            const std::string fromFile = parseUserDir(body, "XDG_MUSIC_DIR");
            if (!fromFile.empty())
                return fromFile;
        }
        return homeDir() + "/Music";
    }();
    return dir;
}

const std::string &exePath()
{
    static const std::string p = [] {
        char buf[4096];
        const ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (n <= 0)
            return std::string();
        buf[n] = '\0';
        return std::string(buf);
    }();
    return p;
}

const std::string &exeDir()
{
    static const std::string d = dirNameOf(exePath());
    return d;
}

} // namespace fs
} // namespace jackbridge
