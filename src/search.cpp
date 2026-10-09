
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <algorithm>

using namespace std;

bool search(string &dir, string &target)
{
    DIR *d = opendir(dir.c_str());
    if (!d)
        return false;

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL)
    {
        string name = entry->d_name;
        if (name == "." or name == "..")
            continue;

        if (name == target)
        {
            closedir(d);
            return true;
        }
        if (entry->d_type == DT_DIR)
        {
            string subdir = dir + "/" + name;
            if (search(subdir, target))
            {
                closedir(d);
                return true;
            }
        }
    }
    closedir(d);
    return false;
};

void handleSearch(const vector<string> &args)
{
    if (args.size() != 2)
    {
        string errMsg = "Usage: search <filename>\n";
        write(STDERR_FILENO, errMsg.c_str(), errMsg.size());
        return;
    }

    string target = args[1];

    // get current working directory
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        perror("getcwd error");
        return;
    }
    string currentDir = cwd;

    bool flag = search(currentDir, target);
    string result = flag ? "True\n" : "False\n";
    write(STDOUT_FILENO, result.c_str(), result.size());
}

