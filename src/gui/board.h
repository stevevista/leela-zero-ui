#pragma once
#include <string>
#include <vector>
#include <array>
#include <iostream>
#include <fstream>


class GoBoard {

public:
    GoBoard(const int bsize = 19);

    int operator[](int i) const { return stones[i]; }

    int board_size() const { return boardsize; }
    void reset(int bsize=0);
    bool update_board(const bool black, const int i);
    bool undo(bool& prev_color, int& prev_pos);

private:
    void merge_strings(const int ip, const int aip);
    void remove_string(int i);

    std::array<int, 361> stones;
    int group_ids[361];
    int group_libs[361];
    int stone_next[361];
    int boardsize;
    std::vector<std::pair<bool,int>> history;
};
