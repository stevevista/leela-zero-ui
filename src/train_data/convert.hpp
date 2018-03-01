#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "Board.h"

using std::vector;


using BoardPlane = std::bitset<361>;

struct MoveData {
    std::vector<BoardPlane> input;
    std::vector<float> probs;
    int result;
};


// DATA format:
// HEADER + LINE * N
// HEADER : 'G', board_size:char
// LINE : 'g' + result:char + move_cnt:uint16 + MOVE * N
// MOVE : uint16  
//        bit0  1 means has leading remove stones, remove_count:uint16 + multiple stone_pos:uint16
//        bit1  1 means has leading distribution values, 362* float
//        bit2  1 means not useful for training
//        bit6 ~ bit15 represents move position  (0x1F & val) 

class GameArchive {

    struct move_t {
        unsigned short pos;
        std::vector<unsigned short> removes;
        std::vector<float> probs;
    };

    struct game_t {
        vector<move_t> seqs;
        int result;
    };

    int boardsize_;
    vector<game_t> games_;
    vector<std::pair<int, int>> entries_; // game_index + step_index

    void extract_move(const int index, const int input_history, MoveData& out);

public:
    int data_index;

    class Writer {
        std::ofstream ofs_;
        std::vector<char> buffer_;
        unsigned short move_count_;

        void push_ushort(unsigned short v);
        void push_float(float v);

    public:
        bool create(const std::string& path, const int board_size);
        void start_game();
        void add_move(const int move, const std::vector<int>& removes, const std::vector<float>& probs, bool valid = true);
        int end_game(int result);
        void close();
    };

    static void generate(const std::string& path, const std::string& out_path, const int expect_boardsize);

    GameArchive(const int bsize = 19);
    void shuffle();
    int total_moves() const { return entries_.size(); }

    int load(const std::string& path, bool append);

    int board_size() const {return boardsize_; }

    vector<MoveData> next_batch(int count, const int input_history, bool& rewinded);
};
