
#include <vector>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <cstdlib>
#include "cd.h"
#include "pwd.h"
#include "echo.h"
#include "ls.h"
#include "pinfo.h"
#include "search.h"
#include "history.h"
#include "ioredirection.h"

using namespace std;

void handlePwd();

// Forward declaration for I/O redirection helper
bool setIORedirection(std::vector<std::string> &args);

bool isBuiltinCommand(const string &cmd)
{
    vector<string> builtinCommands = {"cd", "pwd", "echo", "ls", "pinfo", "search", "exit", "history"};
    return find(builtinCommands.begin(), builtinCommands.end(), cmd) != builtinCommands.end();
}

void executeBuiltinCommand(vector<string> &args)
{
    // Save original stdin and stdout
    int savedStdin = dup(STDIN_FILENO);
    int savedStdout = dup(STDOUT_FILENO);

    if (!setIORedirection(args))
    {
        // If I/O redirection fails, restore original FDs and return
        dup2(savedStdin, STDIN_FILENO);
        dup2(savedStdout, STDOUT_FILENO);
        close(savedStdin);
        close(savedStdout);
        return;
    }

    if (args[0] == "cd")
    {
        handleCd(args);
    }
    else if (args[0] == "pwd")
    {
        handlePwd();
    }
    else if (args[0] == "echo")
    {
        handleEcho(args);
    }
    else if (args[0] == "ls")
    {
        handleLs(args);
    }
    else if (args[0] == "pinfo")
    {
        handlePinfo(args);
    }
    else if (args[0] == "search")
    {
        handleSearch(args);
    }
    else if (args[0] == "history")
    {
        handleHistory(args);
    }
    else if (args[0] == "exit")
    {
        dup2(savedStdin, STDIN_FILENO);
        dup2(savedStdout, STDOUT_FILENO);
        close(savedStdin);
        close(savedStdout);
        exit(0);
    }

    // Restore original stdin and stdout
    dup2(savedStdin, STDIN_FILENO);
    dup2(savedStdout, STDOUT_FILENO);

    close(savedStdin);
    close(savedStdout);
}
