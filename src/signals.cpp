// Required headers
#include <vector>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <iostream>
#include <unordered_set>

using namespace std;

extern pid_t foregroundPid;
extern sig_atomic_t g_sigchld;
extern unordered_set<pid_t> backgroundProcesses;

void reapChildren()
{
    int status;
    pid_t pid;

    // Reap all terminated children (non-blocking)
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if (backgroundProcesses.find(pid) == backgroundProcesses.end())
            continue;

        backgroundProcesses.erase(pid);

        char buf[256];
        int len = 0;

        if (WIFEXITED(status))
        {
            len = snprintf(buf, sizeof(buf),
                           "Background process %d finished with exit code %d\n",
                           pid, WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            len = snprintf(buf, sizeof(buf),
                           "Background process %d killed by signal %d\n",
                           pid, WTERMSIG(status));
        }
        else
        {
            len = snprintf(buf, sizeof(buf),
                           "Background process %d finished\n", pid);
        }

        if (len > 0)
            write(STDOUT_FILENO, buf, len);
    }
}

void sigchldHandler(int)
{
    g_sigchld = 1;
}

void sigintHandler(int)
{
    if (foregroundPid > 0)
    {
        // send SIGINT to whole foreground process group
        kill(-foregroundPid, SIGINT);
    }
}

void sigtstpHandler(int)
{
    if (foregroundPid > 0)
    {
        // send SIGTSTP to whole foreground process group
        kill(-foregroundPid, SIGTSTP);
    }
}
