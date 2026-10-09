#pragma once
#include <vector>
#include <string>
std::vector<std::string> splitCommands(const std::string &input);
std::vector<std::string> tokenizeCommand(const std::string &cmd);
bool checkBackgroundProcess(std::vector<std::string> &args);
