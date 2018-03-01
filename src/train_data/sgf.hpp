#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <map>


class SGFParser {
public:
    
    static void parse(std::istringstream & strm,
        int& boardsize,
        float& komi,
        float& result,
        std::vector<int>& moves);
    static int count_games_in_file(std::string filename);
};

std::vector<std::string> sgf_chop_all(std::string fname,
                                             size_t stopat = SIZE_MAX);
std::vector<int> seq_to_moves(const std::string& seqs, const int boardsize);
