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
    void reset(const int bsize);
    bool update_board(const int color, const int i, std::vector<int>& removed);

    bool update_board(const bool black, const int i) {
        std::vector<int> removed;
        return update_board(black ? 1 : -1, i, removed);
    }

private:
    void merge_strings(const int ip, const int aip);
    void remove_string(int i, std::vector<int>& removed);

    int stones[361];
    int group_ids[361];
    int group_libs[361];
    int stone_next[361];
    int boardsize;
};



// DATA format:
// HEADER + LINE * N
// HEADER : 'G', board_size:char
// LINE : 'g' + size:int(not include self) + result:char + move_cnt:uint16 + MOVE * N
// MOVE : uint16  
//        bit0  1 means has leading remove stones, remove_count:uint16 + multiple stone_pos:uint16
//        bit6 ~ bit15 represents move position  (0x1F & val) 

class GameArchive {
public:
    class Writer {

        GoBoard board_;
        int board_size_;
        bool game_valid_;
        int color_;
        std::ofstream ofs_;
        std::vector<char> buffer_;
        unsigned short move_count_;

        void push_ushort(unsigned short v);

    public:
        bool create(const std::string& path, const int board_size);
        void start_game();
        void add_move(const int move);
        int end_game(int result);
        int encode_game(const std::vector<int>& moves, int result);
        void close(); 
    };

    static void generate(const std::string& path, const std::string& out_path, const int expect_boardsize);
    static int verify(const std::string& path);
};
