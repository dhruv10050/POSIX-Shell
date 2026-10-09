#ifdef __APPLE__
#include <vector>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <iostream>
using namespace std;

#include <unistd.h> // getpid, getpgid, write, STDIN_FILENO
#include <sys/types.h>
#include <termios.h> // tcgetpgrp
#include <errno.h>
#include <string.h>
#include <string>
#include <vector>
#include <stdio.h>         // perror
#include <stdlib.h>        // strtol, getenv
#include <limits.h>        // INT_MAX
#include <libproc.h>       // proc_pidinfo, proc_pidpath, PROC_* constants
#include <sys/proc_info.h> // proc_bsdshortinfo, PROC_PIDT_SHORTBSDINFO

using namespace std;

// Replace $HOME prefix with "~" for pretty printing
static string tildePath(const string &abs)
{
    const char *home = getenv("HOME");
    if (!home)
        return abs;
    size_t n = strlen(home);
    if (abs.size() == n && abs.compare(0, n, home) == 0)
        return "~";
    if (abs.size() > n && abs.compare(0, n, home) == 0 && abs[n] == '/')
        return string("~") + abs.substr(n);
    return abs;
}

// Decide if pid is in this terminal's foreground PGID (append '+')
static bool isForegroundInThisTTY(pid_t pid)
{
    pid_t pgid = getpgid(pid);
    if (pgid <= 0)
        return false;
    pid_t fg = tcgetpgrp(STDIN_FILENO);
    if (fg <= 0)
        return false; // not a TTY / redirected
    return pgid == fg;
}

// Map BSD status to assignment’s letter
// proc_bsdshortinfo.pbsi_status values: SIDL(1), SRUN(2), SSLEEP(3), SSTOP(4), SZOMB(5)
static char map_status(uint8_t s)
{
    switch (s)
    {
    case 2:
        return 'R'; // SRUN
    case 3:
        return 'S'; // SSLEEP
    case 4:
        return 'T'; // SSTOP
    case 5:
        return 'Z'; // SZOMB
    case 1:
    default:
        return 'S'; // SIDL → show as sleeping
    }
}

// Get single-letter status via PROC_PIDT_SHORTBSDINFO
static bool get_status_letter(pid_t pid, char &letter)
{
    struct proc_bsdshortinfo sbi{};
    int r = proc_pidinfo(pid, PROC_PIDT_SHORTBSDINFO, 0, &sbi, sizeof(sbi));
    if (r != (int)sizeof(sbi))
        return false;
    letter = map_status(sbi.pbsi_status);
    return true;
}

// Get virtual memory size in KB via PROC_PIDTASKINFO
static bool get_virtual_memory_kb(pid_t pid, unsigned long long &vm_kb)
{
    struct proc_taskinfo ti{};
    int r = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &ti, sizeof(ti));
    if (r != (int)sizeof(ti))
        return false;
    vm_kb = ti.pti_virtual_size / 1024ULL;
    return true;
}

// Get executable path via proc_pidpath
static bool get_executable_path(pid_t pid, string &path)
{
    char buf[PROC_PIDPATHINFO_MAXSIZE];
    int len = proc_pidpath(pid, buf, sizeof(buf));
    if (len <= 0)
        return false;
    path.assign(buf, (size_t)len);
    return true;
}

// Helper to write to stdout
static void writeln(const string &s)
{
    (void)write(STDOUT_FILENO, s.c_str(), s.size());
}

// Public entry: pinfo [pid]
void handlePinfo(const vector<string> &args)
{
    // Parse PID (optional)
    pid_t pid = 0;
    if (args.size() == 1)
    {
        pid = getpid(); // show info for the shell itself
    }
    else if (args.size() == 2)
    {
        const char *s = args[1].c_str();
        char *end = nullptr;
        errno = 0;
        long v = strtol(s, &end, 10);
        if (errno != 0 || end == s || *end != '\0' || v <= 0 || v > INT_MAX)
        {
            const char *msg = "pinfo: invalid pid\n";
            (void)write(STDERR_FILENO, msg, strlen(msg));
            return;
        }
        pid = static_cast<pid_t>(v);
    }
    else
    {
        const char *msg = "Usage: pinfo [pid]\n";
        (void)write(STDERR_FILENO, msg, strlen(msg));
        return;
    }

    // 1) Status
    char status = '?';
    if (!get_status_letter(pid, status))
    {
        if (errno == ESRCH)
        {
            string err = "pinfo: pid " + to_string(pid) + " not found\n";
            (void)write(STDERR_FILENO, err.c_str(), err.size());
        }
        else
        {
            perror("pinfo: status");
        }
        return;
    }

    // Only R/S can get '+'
    bool plus = (status == 'R' || status == 'S') && isForegroundInThisTTY(pid);

    // 2) Virtual Memory (KB)
    unsigned long long vm_kb = 0;
    if (!get_virtual_memory_kb(pid, vm_kb))
    {
        // Non-fatal; show 0 if unavailable
        vm_kb = 0;
    }

    // 3) Executable Path
    string exe;
    if (!get_executable_path(pid, exe))
    {
        exe = "(unknown)";
    }

    // Print (exact labels / braces / suffix per assignment)
    {
        string line = "Process Status -- {";
        line += status;
        if (plus)
            line += '+';
        line += "}\n";
        writeln(line);
    }
    {
        string line = "memory -- " + to_string(vm_kb) + " {Virtual Memory}\n";
        writeln(line);
    }
    {
        string line = "Executable Path -- " + tildePath(exe) + "\n";
        writeln(line);
    }
}

#else
// Non-Apple platforms: stub to keep build errors obvious if accidentally compiled elsewhere
#include <vector>
#include <string>
#include <string.h>
#include <unistd.h>
void handlePinfo(const vector<string> &)
{
    const char *msg = "pinfo: macOS-only implementation (libproc)\n";
    (void)write(STDERR_FILENO, msg, strlen(msg));
}
#endif
