#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <fstream>


class GoBoard {

public:
    GoBoard(const int bsize = 19);

    int operator[](int i) const { return stones[i]; }

    int board_size() const { return boardsize; }
    void reset(int bsize=0);
    bool update_board(const bool black, const int i);

private:
    void merge_strings(const int ip, const int aip);
    void remove_string(int i);

    int stones[361];
    int group_ids[361];
    int group_libs[361];
    int stone_next[361];
    int boardsize;
};
