#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <unistd.h>

using namespace std;

extern string initialDirectory;

string trim(const string &s)
{
    if (s.empty())
        return "";

    size_t start = 0, end = s.size() - 1;

    while (start < s.size() && isspace((unsigned char)s[start]))
        start++;

    if (start == s.size())
        return "";

    while (end > start && isspace((unsigned char)s[end]))
        end--;

    return s.substr(start, end - start + 1);
}

vector<string> splitCommands(const string &input)
{
    vector<string> commands;
    string cur;

    for (size_t i = 0; i < input.size(); i++)
    {
        char c = input[i];

        if (c == '\'' || c == '"')
        {
            char quote = c;
            cur += c;
            i++;
            while (i < input.size() && input[i] != quote)
            {
                cur += input[i];
                i++;
            }
            if (i < input.size())
            {
                cur += input[i];
            }
        }
        else if (c == ';')
        {
            string trimmed = trim(cur);
            if (!trimmed.empty())
                commands.push_back(trimmed);
            cur.clear();
        }
        else
        {
            cur += c;
        }
    }

    string trimmed = trim(cur);
    if (!trimmed.empty())
        commands.push_back(trimmed);

    return commands;
}

vector<string> tokenizeCommand(const string &command)
{
    vector<string> tokens;
    size_t i = 0, n = command.size();

    while (i < n)
    {
        // handle whitespaces
        while (i < n && isspace((unsigned char)command[i]))
            i++;
        if (i >= n)
            break;

        // handle quotes
        if (command[i] == '"' || command[i] == '\'')
        {
            char quote = command[i++];
            string token;
            while (i < n && command[i] != quote)
            {
                token += command[i++];
            }
            if (i < n && command[i] == quote)
                i++;
            tokens.push_back(token);
            continue;
        }

        // handle pipes
        if (command[i] == '|')
        {
            tokens.push_back("|");
            i++;
            continue;
        }

        // handle redirection operators
        if (command[i] == '<')
        {
            tokens.push_back("<");
            i++;
            continue;
        }
        if (command[i] == '>')
        {
            if (i + 1 < n && command[i + 1] == '>')
            {
                tokens.push_back(">>");
                i += 2;
            }
            else
            {
                tokens.push_back(">");
                i++;
            }
            continue;
        }

        // handle rest
        string token;
        while (i < n && !isspace(command[i]) &&
               command[i] != '<' && command[i] != '>')
        {
            token += command[i++];
        }
        if (!token.empty())
            tokens.push_back(token);
    }

    return tokens;
}

void getInitialDirectory()
{
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        perror("getcwd() error");
        initialDirectory = "";
    }
    else
    {
        initialDirectory = string(cwd);
    }
}

bool checkBackgroundProcess(vector<string> &args)
{
    if (args.empty())
        return false;

    bool isBackgroundProcess = false;

    while (!args.back().empty() && args.back().back() == '&')
    {
        isBackgroundProcess = true;
        args.back().pop_back();

        if (args.back().empty())
        {
            args.pop_back();
            break;
        }
    }

    return isBackgroundProcess;
}
