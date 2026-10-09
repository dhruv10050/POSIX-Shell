
#include <vector>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <limits.h>

using namespace std;

extern string initialDirectory;

void handleCd(const vector<string> &args)
{
    static string previousDirectory;

    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        perror("getcwd error");
        return;
    }
    string currentDirectory(cwd);
    string targetDirectory;

    if (args.size() > 2)
    {
        write(STDOUT_FILENO, "cd: too many arguments\n", 23);
        return;
    }

    if (args.size() == 1)
    {
        targetDirectory = initialDirectory;
    }
    else if (args[1] == "~")
    {
        char *homeDir = getenv("HOME");
        targetDirectory = homeDir ? string(homeDir) : "/";
    }
    else if (args[1] == "-")
    {
        if (previousDirectory.empty())
        {
            write(STDOUT_FILENO, "No previous directory\n", 22);
            return;
        }
        targetDirectory = previousDirectory;
    }
    else
    {
        targetDirectory = args[1];
    }

    if (chdir(targetDirectory.c_str()) != 0)
    {
        perror("cd: ");
        return;
    }

    if (args.size() > 1 && args[1] == "-")
    {
        string displayDirectory = previousDirectory == initialDirectory ? "~" : previousDirectory;
        displayDirectory += "\n";
        write(STDOUT_FILENO, displayDirectory.c_str(), displayDirectory.size());
    }
    previousDirectory = currentDirectory;
}
