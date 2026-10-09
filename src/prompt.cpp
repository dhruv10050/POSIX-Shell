#include <vector>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <iostream>
#include <sys/utsname.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

using namespace std;

extern string initialDirectory;

string getShellPrompt()
{
    auto getUsername = []() -> string
    {
        uid_t uid = getuid();
        struct passwd *pw = getpwuid(uid);
        if (pw && pw->pw_name)
            return string(pw->pw_name);
        return "unknown";
    };

    string userName = getUsername();

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) < 0)
    {
        strcpy(hostname, "host");
    }

    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        perror("getcwd error");
        strcpy(cwd, "?");
    }

    string cwdStr(cwd);
    string prompt = userName + "@" + hostname + " " + (cwdStr == initialDirectory ? "~" : cwdStr) + " % ";
    return prompt;
}
