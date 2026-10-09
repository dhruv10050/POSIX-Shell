#include <iostream>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>
#include <sys/wait.h>
#include <signal.h>
#include <unordered_set>
#include <errno.h>
#include <atomic>
#include <math.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wwrite-strings"
#include <readline/readline.h>
#include <readline/history.h>
#pragma GCC diagnostic pop

#ifdef __cplusplus
extern "C" {
    extern int rl_catch_signals;
    extern int rl_catch_sigwinch;
}
#endif

#include "include/project.h"


using namespace std;

string initialDirectory;
pid_t foregroundPid = -1;                 // treat as PGID of the foreground job
unordered_set<pid_t> backgroundProcesses; // store child pids we want to announce on SIGCHLD
volatile sig_atomic_t g_sigchld = 0; // set in handler, consumed in main loop

int main()
{
    // signal handlers
    if (signal(SIGCHLD, sigchldHandler) == SIG_ERR)
    {
        perror("Failed to set signal handler");
        exit(1);
    }
    if (signal(SIGINT, sigintHandler) == SIG_ERR)
    {
        perror("Failed to set signal handler");
        exit(1);
    }
    if (signal(SIGTSTP, sigtstpHandler) == SIG_ERR)
    {
        perror("Failed to set signal handler");
        exit(1);
    }

    // Disable readline's inbuilt signal handlers
    rl_catch_signals = 0;
    rl_catch_sigwinch = 1;

    // TAB triggers completion
    rl_bind_key('\t', rl_complete);

    // our completion hook
    rl_attempted_completion_function = my_attempted_completion;

    initHistory();

    getInitialDirectory();

    while (true)
    {
        if (g_sigchld)
        {
            g_sigchld = 0;
            reapChildren();
        }

        errno = 0;
        string prompt = getShellPrompt();
        char *line = readline(prompt.c_str());
        if (!line)
        {
            write(STDOUT_FILENO, "\n", 1);
            // interrupted by a signal
            if (errno == EINTR)
            {
                continue;
            }
            break;
        }
        string userInput(line);
        free(line);

        // add to history if it has some non-whitespace characters
        auto not_space = [](unsigned char c)
        { return !isspace(c); };
        if (find_if(userInput.begin(), userInput.end(), not_space) != userInput.end())
        {
            add_history(userInput.c_str()); // enables Up/Down navigation
            save_history_now();             // persists; keeps file at 20 lines
        }

        vector<string> commands = splitCommands(userInput);
        for (string &cmd : commands)
        {
            vector<string> args = tokenizeCommand(cmd);
            bool isBackgroundProcess = checkBackgroundProcess(args);

            if (args.empty())
                continue;

            if (count(args.begin(), args.end(), "|") > 0)
            {
                executePipedCommands(args, isBackgroundProcess);
                continue;
            }

            if (isBuiltinCommand(args[0]))
            {
                executeBuiltinCommand(args);
            }
            else
            {
                executeSystemCommand(args, isBackgroundProcess);
            }

            if (g_sigchld)
            {
                g_sigchld = 0;
                reapChildren();
            }
        }
    }

    return 0;
}
