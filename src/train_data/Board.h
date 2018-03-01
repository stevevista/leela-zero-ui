#pragma once

#include <array>
#include <vector>
#include <iostream>
#include <bitset>


class GoBoard {
    
    static void init_board(const int bsize);
public:

    GoBoard(const int bsize);

    int operator[](int i) const { return stones[i]; }

    void reset();
    bool valid_move(int pos) const { 
        return pos == boardsize*boardsize ||
        (pos >=0 && pos < boardsize*boardsize && !stones[pos]);  
    } // include pass move
    
    void update_board(const int color, const int i, std::vector<int>& removed);

private:
    void merge_strings(const int ip, const int aip);
    void remove_string(int i, std::vector<int>& removed);

    int stones[361];
    int group_ids[361];
    int group_libs[361];
    int stone_next[361];
    const int boardsize;
};
