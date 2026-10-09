#pragma once
#include <vector>
#include <string>
bool isBuiltinCommand(const std::string &cmd);
void executeBuiltinCommand(std::vector<std::string> &args);
