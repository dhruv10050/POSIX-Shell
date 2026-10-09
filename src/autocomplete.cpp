#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <readline/readline.h>
#include <readline/history.h>

using namespace std;

extern "C"
{
    extern int rl_catch_signals;
    extern int rl_catch_sigwinch;
}

// List of built-in commands
static const char *builtinCommands[] = {
    "cd", "pwd", "echo", "ls", "pinfo", "search", "exit", "history", nullptr};

static char *duplicateString(const char *s)
{
    size_t n = strlen(s) + 1;
    char *d = (char *)malloc(n);
    if (d)
        memcpy(d, s, n);
    return d;
}

// Called repeatedly by readline to produce successive matches for `text`.
static char *builtinGenerator(const char *text, int state)
{
    static int idx;
    if (state == 0)
        idx = 0;

    const char *cand;
    size_t L = strlen(text);
    while ((cand = builtinCommands[idx++]))
    {
        if (strncmp(cand, text, L) == 0)
        {
            return duplicateString(cand); // readline will free this
        }
    }
    return nullptr;
}

// Main hook: first token -> builtins (with fallback); others -> filenames
char **my_attempted_completion(const char *text, int start, int end)
{
    if (end < 0)
        return nullptr;

    if (start == 0)
    {
        // try builtins
        char **matches = rl_completion_matches(text, builtinGenerator);
        if (matches)
            return matches;

        // no builtin matches -> allow default filename completion
        return nullptr;
    }
    // not first token -> default filename completion
    return nullptr;
}
