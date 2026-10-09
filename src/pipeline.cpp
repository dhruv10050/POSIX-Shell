// Required headers
#include <vector>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <unordered_set>

using namespace std;

bool setIORedirection(vector<string> &args);
extern unordered_set<pid_t> backgroundProcesses;
extern pid_t foregroundPid;

void executePipedCommands(vector<string> &args, bool isBackgroundProcess)
{
    vector<vector<string>> commands;
    auto splitByPipe = [&]() -> bool
    {
        vector<string> currentCommand;
        for (const string &arg : args)
        {
            if (arg == "|")
            {
                if (currentCommand.empty())
                {
                    const char *msg = "syntax error near unexpected token '|'\n";
                    write(STDERR_FILENO, msg, strlen(msg));
                    return false;
                }
                commands.push_back(currentCommand);
                currentCommand.clear();
            }
            else
            {
                currentCommand.push_back(arg);
            }
        }
        if (currentCommand.empty())
        {
            const char *msg = "syntax error near unexpected token '|'\n";
            write(STDERR_FILENO, msg, strlen(msg));
            return false;
        }
        commands.push_back(currentCommand);
        return true;
    };

    if (!splitByPipe())
        return;

    int numCmds = commands.size();
    int pipefd[2], inputFd = STDIN_FILENO;
    vector<pid_t> pids;
    pid_t pgid = -1;

    for (int i = 0; i < numCmds; i++)
    {
        if (i < numCmds - 1)
        {
            if (pipe(pipefd) < 0)
            {
                perror("pipe failed");
                return;
            }
        }

        pid_t pid = fork();
        if (pid < 0)
        {
            perror("fork failed");
            return;
        }

        if (pid == 0)
        {
            // ---- Child ----
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            if (i == 0)
                setpgid(0, 0); // leader
            else
                setpgid(0, pgid); // join group

            // Input from previous pipe
            if (inputFd != STDIN_FILENO)
            {
                dup2(inputFd, STDIN_FILENO);
                close(inputFd);
            }

            // Output to next pipe
            if (i < numCmds - 1)
            {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
            }

            // IO redirection inside stage (first/last)
            vector<string> tempArgs = commands[i];
            if (!setIORedirection(tempArgs))
                _exit(1);

            vector<char *> argv;
            for (string &arg : tempArgs)
                argv.push_back((char *)arg.c_str());
            argv.push_back(nullptr);

            execvp(argv[0], argv.data());
            perror("execvp failed");
            _exit(1);
        }
        else
        {
            // ---- Parent ----
            if (i == 0)
            {
                setpgid(pid, pid); // establish leader
                pgid = pid;
            }
            else
            {
                setpgid(pid, pgid);
            }

            pids.push_back(pid);

            if (inputFd != STDIN_FILENO)
                close(inputFd);

            if (i < numCmds - 1)
            {
                close(pipefd[1]);
                inputFd = pipefd[0];
            }
        }
    }

    if (inputFd != STDIN_FILENO)
        close(inputFd); // small leak fix

    if (isBackgroundProcess)
    {
        // Track all children so SIGCHLD prints completions
        for (pid_t pid : pids)
            backgroundProcesses.insert(pid);

        string msg = "Started background pipeline with PID " + to_string(pids.back()) + "\n";
        write(STDOUT_FILENO, msg.c_str(), msg.size());
    }
    else
    {
        // Foreground pipeline: wait for the group; return when stopped or finished
        foregroundPid = pgid;

        while (true)
        {
            int status = 0;
            pid_t w = waitpid(-foregroundPid, &status, WUNTRACED);
            if (w < 0)
            {
                if (errno == EINTR)
                    continue;
                if (errno == ECHILD)
                    break; // group finished
                perror("waitpid");
                break;
            }
            if (WIFSTOPPED(status))
            {
                // move whole pipeline to background as stopped
                for (pid_t pid : pids)
                    backgroundProcesses.insert(pid);
                const char *msg = "Suspended and moved to background\n";
                write(STDOUT_FILENO, msg, strlen(msg));
                break;
            }
            // else one child exited; continue until ECHILD or stopped
        }
        foregroundPid = -1;
    }
}
