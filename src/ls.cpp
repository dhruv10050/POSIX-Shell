#include <vector>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctime>
#include <cstring>

using namespace std;

void handleLs(const vector<string> &args)
{
    bool showAll = false, longList = false;
    bool endOfFlags = false, invalidFlag = false;
    vector<string> paths;
    vector<vector<string>> fileInfoList;

    auto parseArgs = [&](const vector<string> &args) -> void
    {
        for (size_t i = 1; i < args.size(); i++)
        {
            const string &arg = args[i];
            if (!endOfFlags && arg == "--")
            {
                endOfFlags = true;
            }
            else if (!endOfFlags && arg.size() > 1 && arg[0] == '-')
            {
                for (char c : arg.substr(1))
                {
                    if (c == 'a')
                        showAll = true;
                    else if (c == 'l')
                        longList = true;
                    else
                    {
                        invalidFlag = true;
                        string m = "ls: invalid option -- '";
                        m += c;
                        m += "'\n";
                        write(STDERR_FILENO, m.c_str(), m.size());
                    }
                }
            }
            else if (arg == "~")
            {
                const char *home = getenv("HOME");
                paths.push_back(home ? string(home) : ".");
            }
            else
            {
                paths.push_back(arg);
            }
        }
        if (paths.empty())
            paths.push_back(".");
    };

    auto storeFileInfo = [&](const string &name, const struct stat &st = {}) -> void
    {
        vector<string> fileInfo;
        if (longList)
        {
            char perms[11];
            perms[0] =
                S_ISDIR(st.st_mode) ? 'd' : S_ISLNK(st.st_mode) ? 'l'
                                        : S_ISCHR(st.st_mode)   ? 'c'
                                        : S_ISBLK(st.st_mode)   ? 'b'
                                        : S_ISSOCK(st.st_mode)  ? 's'
                                        : S_ISFIFO(st.st_mode)  ? 'p'
                                                                : '-';
            perms[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
            perms[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
            perms[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';
            perms[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
            perms[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
            perms[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';
            perms[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
            perms[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
            perms[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';
            perms[10] = '\0';

            struct passwd *pw = getpwuid(st.st_uid);
            struct group *gr = getgrgid(st.st_gid);

            char timebuf[80];
            std::tm *tmv = std::localtime(&st.st_mtime);
            if (!tmv)
            {
                timebuf[0] = '\0';
            }
            else
            {
                strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tmv);
            }

            fileInfo.push_back(string(perms));
            fileInfo.push_back(to_string(st.st_nlink));
            fileInfo.push_back(pw ? pw->pw_name : "?");
            fileInfo.push_back(gr ? gr->gr_name : "?");
            fileInfo.push_back(to_string(st.st_size));
            fileInfo.push_back(timebuf);
            fileInfo.push_back(name);
        }
        else
        {
            fileInfo.push_back(name);
        }
        fileInfoList.push_back(fileInfo);
    };

    auto executeLs = [&](const string &path) -> void
    {
        DIR *dir = opendir(path.c_str());
        if (!dir)
        {
            struct stat st;
            if (lstat(path.c_str(), &st) != 0)
            {
                string m = "ls: cannot access '" + path + "': No such file or directory\n";
                write(STDERR_FILENO, m.c_str(), m.size());
            }
            else
            {
                if (longList)
                    storeFileInfo(path, st);
                else
                    storeFileInfo(path);
            }
            return;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            string filename = entry->d_name;
            if (!showAll && !filename.empty() && filename[0] == '.')
                continue;

            if (longList)
            {
                struct stat st;
                string filePath = (path == "/") ? "/" : (path + "/"); // fix root join
                if (lstat((filePath + filename).c_str(), &st) != 0)
                {
                    string m = "ls: cannot access '" + filename + "': No such file or directory\n";
                    write(STDERR_FILENO, m.c_str(), m.size());
                }
                else
                {
                    storeFileInfo(filename, st);
                }
            }
            else
            {
                storeFileInfo(filename);
            }
        }
        closedir(dir);
    };

    auto formatFileInfo = [&]() -> void
    {
        int rows = (int)fileInfoList.size(), cols = longList ? 7 : 1;
        vector<int> maxColWidth(cols, 0);

        for (int i = 0; i < rows; ++i)
            for (int j = 0; j < cols; ++j)
                maxColWidth[j] = max(maxColWidth[j], (int)fileInfoList[i][j].size());

        sort(fileInfoList.begin(), fileInfoList.end(),
             [&](const vector<string> &a, const vector<string> &b)
             {
                 return a[longList ? 6 : 0] < b[longList ? 6 : 0];
             });

        for (int i = 0; i < rows; ++i)
        {
            for (int j = 0; j < cols; ++j)
            {
                string &cell = fileInfoList[i][j];
                if ((int)cell.size() < maxColWidth[j])
                    cell.append(maxColWidth[j] - cell.size(), ' ');
                if (j != cols - 1)
                    cell += "  ";
            }
        }
    };

    parseArgs(args);
    if (invalidFlag)
        return;

    for (size_t i = 0; i < paths.size(); i++)
    {
        fileInfoList.clear();
        const string &path = paths[i];

        if (paths.size() > 1)
        {
            if (i > 0)
                write(STDOUT_FILENO, "\n", 1);
            string header = path + ":\n";
            write(STDOUT_FILENO, header.c_str(), header.size());
        }

        executeLs(path);
        formatFileInfo();

        for (const auto &fileInfo : fileInfoList)
        {
            for (const auto &cell : fileInfo)
                write(STDOUT_FILENO, cell.c_str(), cell.size());
            write(STDOUT_FILENO, "\n", 1);
        }
    }
}
