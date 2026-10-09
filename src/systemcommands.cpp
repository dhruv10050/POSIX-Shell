// Required headers
#include <vector>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <iostream>
#include <unordered_set>

using namespace std;

bool setIORedirection(vector<string> &args);
extern unordered_set<pid_t> backgroundProcesses;
extern pid_t foregroundPid;

void executeSystemCommand(vector<string> &args, bool isBackgroundProcess)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork failed");
        return;
    }

    if (pid == 0)
    {
        // ---- Child ----
        // restore default handlers so child receives Ctrl-C/Z normally
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        // put child in its own process group (leader)
        setpgid(0, 0);

        if (!setIORedirection(args))
            _exit(1);

        vector<char *> cargs;
        for (string &arg : args)
            cargs.push_back(const_cast<char *>(arg.c_str()));
        cargs.push_back(nullptr);

        execvp(cargs[0], cargs.data());
        perror("execvp failed");
        _exit(1);
    }
    else
    {
        // ---- Parent ----
        // ensure group is set (avoid race by calling in parent too)
        setpgid(pid, pid);
        pid_t pgid = pid;

        if (isBackgroundProcess)
        {
            // Track the child PID so SIGCHLD will announce completion
            backgroundProcesses.insert(pid);

            string msg = "Started background process with PID " + to_string(pid) + "\n";
            write(STDOUT_FILENO, msg.c_str(), msg.size());
        }
        else
        {
            // Foreground job: wait for group; return on stop as well
            int status;
            foregroundPid = pgid;

            if (waitpid(-foregroundPid, &status, WUNTRACED) < 0)
            {
                if (errno != ECHILD)
                    perror("waitpid");
            }
            else if (WIFSTOPPED(status))
            {
                // move to background as stopped
                backgroundProcesses.insert(pid);
                const char *msg = "Suspended and moved to background\n";
                write(STDOUT_FILENO, msg, strlen(msg));
            }

            foregroundPid = -1;
        }
    }
}
