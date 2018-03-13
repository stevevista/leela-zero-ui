#pragma once

#include <string>
#include <vector>

std::string findPossibleWeightsFile(const std::string &directory);
void parseLeelaZeroArgs(int argc, char **argv, std::vector<std::string>& players);
