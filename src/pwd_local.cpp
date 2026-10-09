#include <vector>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <limits.h>

using namespace std;

extern string initialDirectory;

void handlePwd()
{
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        perror("getcwd error");
        return;
    }
    string currentDirectory(cwd);
    string displayDirectory = currentDirectory == initialDirectory ? "~" : currentDirectory;
    displayDirectory += "\n";
    write(STDOUT_FILENO, displayDirectory.c_str(), displayDirectory.size());
}
