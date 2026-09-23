// See proc.h.

#include "proc.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern char **environ;

namespace jackbridge
{
namespace proc
{

namespace
{

// A NUL-terminated char* array over strings the CALLER still owns. Built before fork(), because
// everything between fork() and exec() must be async-signal-safe and allocating is not.
std::vector<char *> toCArgv(const Argv &argv)
{
    std::vector<char *> out;
    out.reserve(argv.size() + 1);
    for (const std::string &s : argv)
        out.push_back(const_cast<char *>(s.c_str()));
    out.push_back(nullptr);
    return out;
}

// Build the child's environment as a NUL-terminated array BEFORE forking, for the same reason.
std::vector<std::string> buildEnv(const EnvPairs &extra)
{
    std::vector<std::string> env;
    for (char **e = environ; e && *e; ++e) {
        const char *eq = strchr(*e, '=');
        if (!eq) {
            env.emplace_back(*e);
            continue;
        }
        const std::string name(*e, static_cast<size_t>(eq - *e));
        // An entry we are about to set is dropped here rather than appended twice: a duplicate in
        // envp is not an error, but which one the child sees is left to the libc, and a child
        // reading the stale one would act on the previous device.
        bool overridden = false;
        for (const auto &kv : extra) {
            if (kv.first == name) {
                overridden = true;
                break;
            }
        }
        if (!overridden)
            env.emplace_back(*e);
    }
    for (const auto &kv : extra)
        env.push_back(kv.first + "=" + kv.second);
    return env;
}

// Put the child back to a known signal disposition. It inherits this process's SIGCHLD handler and
// any mask, and a child that runs with jack-bridge's handler installed is a child whose own
// children go unreaped.
void resetSignalsInChild()
{
    signal(SIGCHLD, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGPIPE, SIG_DFL);
    sigset_t empty;
    sigemptyset(&empty);
    sigprocmask(SIG_SETMASK, &empty, nullptr);
}

} // namespace

//------------------------------------------------------------------------
pid_t spawnAsync(const Argv &argv)
{
    if (argv.empty() || argv[0].empty())
        return -1;

    std::vector<char *> cargv = toCArgv(argv);

    const pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "jack-bridge: cannot fork to run %s (%s)\n", argv[0].c_str(),
                strerror(errno));
        return -1;
    }
    if (pid == 0) {
        resetSignalsInChild();
        execvp(cargv[0], cargv.data());
        // Only reached when exec failed. 127 is the shell's convention for "not found" and is what
        // the callers' previous g_spawn paths reported, so the caller's own handling is unchanged.
        _exit(127);
    }
    return pid;
}

//------------------------------------------------------------------------
int runCapture(const Argv &argv, std::string &out)
{
    out.clear();
    if (argv.empty() || argv[0].empty())
        return -1;

    int pfd[2];
    if (pipe2(pfd, O_CLOEXEC) != 0) {
        fprintf(stderr, "jack-bridge: cannot create a pipe for %s (%s)\n", argv[0].c_str(),
                strerror(errno));
        return -1;
    }

    std::vector<char *> cargv = toCArgv(argv);

    const pid_t pid = fork();
    if (pid < 0) {
        close(pfd[0]);
        close(pfd[1]);
        fprintf(stderr, "jack-bridge: cannot fork to run %s (%s)\n", argv[0].c_str(),
                strerror(errno));
        return -1;
    }

    if (pid == 0) {
        // --- child ---
        close(pfd[0]);
        // The pipe becomes stdout. dup2 clears O_CLOEXEC on the new descriptor, which is what makes
        // it survive the exec; pfd[1] itself still has it and closes.
        if (dup2(pfd[1], STDOUT_FILENO) < 0)
            _exit(127);
        // Standard error to /dev/null, matching the 2>/dev/null on every pipeline this replaces.
        // Not merged into stdout: `aplay -l` on a machine with no cards writes an explanation
        // there, and parsing it as card output is how a diagnostic becomes a device.
        const int devnull = open("/dev/null", O_WRONLY | O_CLOEXEC);
        if (devnull >= 0) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        close(pfd[1]);
        resetSignalsInChild();
        execvp(cargv[0], cargv.data());
        _exit(127);
    }

    // --- parent ---
    close(pfd[1]);

    // READ TO EOF BEFORE WAITING. The other order deadlocks the moment a child writes more than a
    // pipe buffer: it blocks in write(), we block in waitpid(), and neither moves. Today's callers
    // produce a few hundred bytes, which is not a reason to write it the wrong way round.
    char buf[4096];
    for (;;) {
        const ssize_t n = read(pfd[0], buf, sizeof(buf));
        if (n > 0) {
            out.append(buf, static_cast<size_t>(n));
            continue;
        }
        if (n < 0 && errno == EINTR)
            continue;
        break;
    }
    close(pfd[0]);

    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR)
            continue;
        // ECHILD here would mean something else reaped it. wakepipe only ever reaps pids it was
        // asked to watch, precisely so this cannot happen -- see wakepipe.h. Report the failure
        // rather than a made-up success.
        fprintf(stderr, "jack-bridge: lost the exit status of %s (%s)\n", argv[0].c_str(),
                strerror(errno));
        return -1;
    }

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    return -1;
}

int run(const Argv &argv)
{
    std::string ignored;
    return runCapture(argv, ignored);
}

//------------------------------------------------------------------------
pid_t spawnDetached(const Argv &argv, const std::string &cwd, const EnvPairs &extraEnv)
{
    if (argv.empty() || argv[0].empty())
        return -1;

    // RESOLVED IN THE PARENT, where allocating is still legal, so the grandchild can use execve
    // with an absolute path. execvpe() would do the search itself, but it is a GNU extension and
    // needs _GNU_SOURCE; doing it here needs neither, and a program that is not installed is then
    // reported as such instead of as a grandchild that exits 127 a moment after we returned its
    // pid.
    const std::string exe = findOnPath(argv[0]);
    if (exe.empty()) {
        fprintf(stderr, "jack-bridge: %s is not installed or not executable\n", argv[0].c_str());
        return -1;
    }
    Argv resolved = argv;
    resolved[0] = exe;

    const std::vector<std::string> envStrings = buildEnv(extraEnv);
    std::vector<char *> envp;
    envp.reserve(envStrings.size() + 1);
    for (const std::string &s : envStrings)
        envp.push_back(const_cast<char *>(s.c_str()));
    envp.push_back(nullptr);

    std::vector<char *> cargv = toCArgv(resolved);

    // O_CLOEXEC so the write end closes automatically on a successful exec.
    int pfd[2];
    if (pipe2(pfd, O_CLOEXEC) != 0)
        return -1;

    const pid_t mid = fork();
    if (mid < 0) {
        close(pfd[0]);
        close(pfd[1]);
        return -1;
    }

    if (mid == 0) {
        // --- intermediate child ---
        close(pfd[0]);

        // Its own session, so a signal to the GUI's process group does not reach the bridge.
        setsid();

        const pid_t gc = fork();
        if (gc == 0) {
            // --- grandchild: becomes the bridge ---
            close(pfd[1]);
            if (!cwd.empty() && chdir(cwd.c_str()) != 0) {
                // Not fatal: everything here is started by absolute path and reads nothing relative
                // to the working directory. Carry on rather than lose the audio.
            }
            resetSignalsInChild();
            // SIGUSR1 as well: the bridge uses it for the live device switch, and it must start
            // from the default disposition rather than whatever the GUI had.
            signal(SIGUSR1, SIG_DFL);
            execve(cargv[0], cargv.data(), envp.data());
            _exit(127); // exec failed; the pid the parent got will simply not stay alive
        }

        // Report the grandchild's pid (or -1) and get out of the way, orphaning it onto init.
        const long outPid = (gc < 0) ? -1 : static_cast<long>(gc);
        const ssize_t ignored = write(pfd[1], &outPid, sizeof(outPid));
        (void)ignored;
        close(pfd[1]);
        _exit(0);
    }

    // --- parent ---
    close(pfd[1]);

    long pid = -1;
    ssize_t n;
    do {
        n = read(pfd[0], &pid, sizeof(pid));
    } while (n < 0 && errno == EINTR);
    close(pfd[0]);

    // Reap the intermediate immediately; it has already exited or is about to. Waited on by pid, so
    // it cannot take another child's status.
    int status = 0;
    while (waitpid(mid, &status, 0) < 0 && errno == EINTR) {
    }

    if (n != static_cast<ssize_t>(sizeof(pid)))
        return -1;
    return static_cast<pid_t>(pid);
}

//------------------------------------------------------------------------
bool pidAlive(pid_t pid)
{
    if (pid <= 0)
        return false;
    // ESRCH means gone; EPERM means it exists and belongs to somebody else, which still counts as
    // alive. A zombie answers success here, which is why spawnDetached() takes care not to leave
    // one behind.
    return kill(pid, 0) == 0 || errno == EPERM;
}

//------------------------------------------------------------------------
std::string findOnPath(const std::string &program)
{
    if (program.empty())
        return std::string();
    if (program.find('/') != std::string::npos)
        return access(program.c_str(), X_OK) == 0 ? program : std::string();

    const char *path = getenv("PATH");
    // The same fallback execvp uses when PATH is unset (confstr(_CS_PATH) in glibc), spelled out
    // rather than left to differ from what the exec would actually search.
    const std::string search = path && *path ? path : "/usr/local/bin:/usr/bin:/bin";

    size_t start = 0;
    while (start <= search.size()) {
        size_t end = search.find(':', start);
        if (end == std::string::npos)
            end = search.size();
        // An empty entry means the current directory, as PATH is specified -- honoured for the same
        // reason the fallback above is: this has to agree with the exec, not with what would be
        // tidier.
        std::string dir = search.substr(start, end - start);
        if (dir.empty())
            dir = ".";
        const std::string full = dir + "/" + program;
        if (access(full.c_str(), X_OK) == 0) {
            struct stat st;
            if (stat(full.c_str(), &st) == 0 && S_ISREG(st.st_mode))
                return full;
        }
        if (end == search.size())
            break;
        start = end + 1;
    }
    return std::string();
}

} // namespace proc
} // namespace jackbridge
