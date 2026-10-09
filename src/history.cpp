// Required headers
#include <vector>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <iostream>

using namespace std;

void handleHistory(const vector<string> &args)
{
    // default: print last 10
    int max_to_show = args.size() > 1 ? stoi(args[1]) : 10;
    if (max_to_show < 0)
        max_to_show = 0;
    if (max_to_show > 20)
        max_to_show = 20;

    HIST_ENTRY **list = history_list();
    if (!list)
        return;

    int len = history_length; // total entries
    int start = max(0, len - max_to_show);
    for (int i = start; i < len; ++i)
    {
        const char *s = list[i]->line ? list[i]->line : "";
        write(STDOUT_FILENO, s, strlen(s));
        write(STDOUT_FILENO, "\n", 1);
    }
}

// ---- History helpers ----
static string historyFilePath()
{
    const char *home = getenv("HOME");
    if (!home)
        home = ".";
    return string(home) + "/.mysh_history";
}

void initHistory()
{
    using_history();
    read_history(historyFilePath().c_str()); // load previous sessions
    stifle_history(20);                      // cap to 20 in memory
}

void save_history_now()
{
    history_truncate_file(historyFilePath().c_str(), 20); // trim file if it exists
    write_history(historyFilePath().c_str());             // rewrite full in-mem history
}
