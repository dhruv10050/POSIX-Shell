// Required headers
#include <vector>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

using namespace std;

bool setIORedirection(vector<string> &args)
{
    string inputFile, outputFile;
    bool append = false;

    vector<string> parsedArgs;

    for (int i = 0; i < (int)args.size(); i++)
    {
        if (args[i] == "<" && i + 1 < (int)args.size())
        {
            inputFile = args[i + 1];
            i++;
        }
        else if (args[i] == ">" && i + 1 < (int)args.size())
        {
            outputFile = args[i + 1];
            append = false;
            i++;
        }
        else if (args[i] == ">>" && i + 1 < (int)args.size())
        {
            outputFile = args[i + 1];
            append = true;
            i++;
        }
        else
        {
            parsedArgs.push_back(args[i]); // keep normal args
        }
    }
    args = parsedArgs;

    if (!inputFile.empty())
    {
        int fd_in = open(inputFile.c_str(), O_RDONLY);
        if (fd_in < 0)
        {
            perror("input redirection failed");
            return false;
        }
        dup2(fd_in, STDIN_FILENO);
        close(fd_in);
    }
    if (!outputFile.empty())
    {
        int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
        int fd_out = open(outputFile.c_str(), flags, 0644);
        if (fd_out < 0)
        {
            perror("output redirection failed");
            return false;
        }
        dup2(fd_out, STDOUT_FILENO);
        close(fd_out);
    }
    return true;
}
