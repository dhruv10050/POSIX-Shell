
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>

using namespace std;

void handleEcho(const vector<string> &args)
{
    if (args.size() < 2)
    {
        write(STDOUT_FILENO, "\n", 1);
        return;
    }

    string message;
    for (size_t i = 1; i < args.size(); ++i)
    {
        message += args[i];
        if (i < args.size() - 1)
            message += " ";
    }
    message += "\n";
    write(STDOUT_FILENO, message.c_str(), message.size());
}
