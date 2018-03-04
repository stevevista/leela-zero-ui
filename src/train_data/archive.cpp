#include "archive.hpp"
#include "sgf.hpp"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cstring>
#include <map>
#include <cassert>
#include "utils.hpp"


using namespace napp;


static constexpr int64_t MAX_DATA_SIZE_KB = 12 * 1024 * 1024;
static constexpr float score_thres = 2.5;
static constexpr int move_count_thres = 30;


bool GameArchive::Writer::create(const std::string& path, const int board_size) {

    ofs_ = std::ofstream(path, std::ofstream::binary);
    if (!ofs_)
        return false;

    ofs_.write("G", 1);
    char c = (char)board_size;
    ofs_.write(&c, 1);
    board_size_ = board_size;
    return true;
}

void GameArchive::Writer::start_game() {

    buffer_.clear();
    move_count_ = 0;
    board_.reset(board_size_);
    game_valid_ = true;
    color_ = 1;

    buffer_.emplace_back('g');

    buffer_.emplace_back('x');  // palceholder for size
    buffer_.emplace_back('x');
    buffer_.emplace_back('x');
    buffer_.emplace_back('x');

    buffer_.emplace_back('x'); // palceholder for result

    buffer_.emplace_back('x'); // palceholder for count
    buffer_.emplace_back('x');
}


int GameArchive::Writer::end_game(int result) {

    if (!game_valid_) {
        std::cerr << "game sequences discard (invalid moves)" << std::endl;
        return 0;
    }

    if (buffer_.empty()) {
        throw std::runtime_error("should call GameArchive::Writer::start_game");
    }

    int wrt_size = buffer_.size();
    int size = wrt_size - 5;

    std::memcpy(&buffer_[1], &size, 4);
    buffer_[5] = (char)result;
    std::memcpy(&buffer_[6], &move_count_, 2);
    ofs_.write(&buffer_[0], wrt_size);
    
    game_valid_ = false;
    buffer_.clear();
    return wrt_size;
}


void GameArchive::Writer::push_ushort(unsigned short v) {
    buffer_.emplace_back('x'); // palceholder for count
    buffer_.emplace_back('x');
    std::memcpy(&buffer_[buffer_.size()-2], &v, 2);
}

void GameArchive::Writer::push_float(float v) {
    buffer_.emplace_back('x'); // palceholder
    buffer_.emplace_back('x');
    buffer_.emplace_back('x');
    buffer_.emplace_back('x');
    std::memcpy(&buffer_[buffer_.size()-4], &v, 4);
}


void GameArchive::Writer::add_move(const int move, const std::vector<float>& probs, bool valid) {

    if (!game_valid_) {
        return;
    }

    std::vector<int> removes;
    // test it 
    if (!board_.update_board(color_, move, removes)) {
        game_valid_ = false;
        return;
    }

    unsigned short move_val = move;
    if (removes.size()) move_val |= 0x8000;
    if (probs.size()) move_val |= 0x4000;
    if (!valid) move_val |= 0x2000;

    push_ushort(move_val);

    if (removes.size()) {
        push_ushort(removes.size());
        for (auto rm : removes)
            push_ushort(rm);
    }

    if (probs.size()) {
        for (auto p : probs)
            push_float(p);
    }

    color_ = -color_;
    move_count_++;
}


int GameArchive::Writer::encode_game(const std::vector<int>& moves, int result) {
    
    start_game();

    for (auto idx : moves) {
        add_move(idx, {});
    }

    return end_game(result);
}

void GameArchive::Writer::close() {
    ofs_.close();
}

static void proceed_moves(GameArchive::Writer& writer, int result, std::vector<int> tree_moves,
    const int64_t total_games,
    const int64_t prcessed, 
    int64_t& total_moves,
    int64_t& total_size,
    const int expect_boardsize) {

    if (tree_moves.size() >=2 && 
        tree_moves[tree_moves.size()-2] == expect_boardsize*expect_boardsize && 
        tree_moves[tree_moves.size()-1] == expect_boardsize*expect_boardsize)
        tree_moves.erase(tree_moves.end()-1);

    if (tree_moves.size() < move_count_thres)
        return;

    int wrt_size = writer.encode_game(tree_moves, result);
    if (wrt_size == 0) {
        return; // invalid game
    }

    int move_count = tree_moves.size();
    total_moves += move_count;
    total_size += wrt_size;

    int percent = (prcessed*100)/total_games;
    float gb = float(total_size)/(1024*1024*1024);
    std::cout << percent << "%, " << "commiting " << move_count << ", total moves " << total_moves << ", size " << gb << "GB" << std::endl;
}

static void parse_sgf_file(GameArchive::Writer& writer, const std::string& sgf_name, 
    int64_t& total_games,
    int64_t& prcessed,
    int64_t& total_moves,
    int64_t& total_size,
    const int expect_boardsize) {

    auto games = sgf_chop_all(sgf_name);
    if (games.size() > 1)
        total_games += (games.size() - 1);

    for (const auto& s : games) {
            
        prcessed++;
            
        int boardsize;
        float komi;
        float result;
        std::vector<int> moves;

        std::istringstream pstream(s);
        SGFParser::parse(pstream, boardsize, komi, result, moves);

        if (boardsize != expect_boardsize) {
            std::cerr << "boarsize " << boardsize << " is not expected" << std::endl;
            continue;
        }
            
        if (moves.size() < 0)
            continue;

        if (result == 0)
            continue;

        auto fixed_result = result - (7.5 - komi);
        if (std::abs(fixed_result) < score_thres)
            continue;

        int r = fixed_result > 0 ? 1 : -1;

        proceed_moves(writer, r, moves,
            total_games,
            prcessed, 
            total_moves,
            total_size,
            expect_boardsize);

        if (total_size/1024 > MAX_DATA_SIZE_KB)
            break;
    }
}


static void parse_index(const std::string& path, std::map<uint32_t, int>& scores) {

    std::ifstream ifs(path);
    
    auto read_nation = [&]() {
            std::string tmp;
            do {
                ifs >> tmp;
                if (ifs.eof()) break;
                if (tmp.size()==1 && tmp[0]>='0' && tmp[0]<='5')
                    break;
            } while(true);
    };
    
    while (!ifs.eof()) {
            
            uint32_t id;
            std::string date;
            std::string time;
            std::string white;
            std::string white_eng;
            std::string black;
            std::string black_eng;
            std::string result;
            int round;
            std::string byoyomi; 
            std::string minutes;
    
            ifs >> id;
            ifs >> date;
    
            //std::cout << date << std::endl;
    
            if (ifs.eof()) break;
    
            ifs >> time;
            ifs >> white;
            ifs >> white_eng;
            read_nation();
            ifs >> black;
            ifs >> black_eng;
            read_nation();
            ifs >> result;
            ifs >> round;
            ifs >> byoyomi;
            ifs >> minutes;
    
            int score = 0;
            if (result.find("B+") == 0)
                score = 1;
            else if (result.find("W+") == 0)
                score = -1;
            else 
                continue;
    
            auto reason = result.substr(2);
            
            auto v = std::atof(reason.c_str());
            if (v == 0) {
                if (reason != "Resign")
                    continue;
            } else {
                if (std::abs(v*score-1) < score_thres)
                    continue; // since default komi == 6.5 
            }
    
            scores.insert({id, score});
        }
        std::cout << "done" << std::endl;
}

        
static int parse_kifu_moves(const std::string& path, const std::map<uint32_t, int>& scores, GameArchive::Writer& writer, 
    int64_t total_games,
    int64_t& prcessed, 
    int64_t& total_moves, 
    int64_t& total_size) {

    std::ifstream ifs(path);
    std::cout << path << std::endl;

    int total = 0;

    while (!ifs.eof()) {
        uint32_t id;
        std::string seqs;

        ifs >> id;
        if (ifs.eof()) break;

        ifs >> seqs;

        auto it = scores.find(id);
        if (it == scores.end())
            continue;

        prcessed++;

        int result = it->second;

        auto tree_moves = seq_to_moves(seqs, 19);

        proceed_moves(writer, result, tree_moves,
                    total_games,
                    prcessed, 
                    total_moves,
                    total_size,
                    19);

        if (total_size/1024 > MAX_DATA_SIZE_KB)
            break;
    }

    return total;
}


void GameArchive::generate(const std::string& path, const std::string& out_path, const int expect_boardsize) {

    std::vector<std::string> file_list;
    napp::enumerateDirectory(file_list, path, true, {});

    std::map<uint32_t, int> scores;
    int sgf_counts = 0;
    
    for (auto f : file_list) {
    
        auto ext = f.substr(f.rfind(".")+1);
        if (ext == "index" && expect_boardsize == 19) 
            parse_index(f, scores);
    
        if (ext == "sgf") 
            sgf_counts++;
    }

    Writer writer;
    writer.create(out_path, expect_boardsize);

    int64_t prcessed = 0;
    int64_t total_moves = 0;
    int64_t total_games = scores.size() + sgf_counts;
    int64_t total_size = 1;
    
    for (auto f : file_list) {
        
        auto ext = f.substr(f.rfind(".")+1);
        if (ext == "index")
            continue;
        
        if (ext == "txt" || ext == "sgf") {
            std::cout << f << std::endl;
            parse_sgf_file(writer, f, total_games, prcessed, total_moves, total_size, expect_boardsize); 
        } else {
            if (expect_boardsize == 19)
                parse_kifu_moves(f, scores, writer, total_games, prcessed, total_moves, total_size);
        }
        
        if (total_size/1024 > MAX_DATA_SIZE_KB)
            break;
    }

    writer.close();
}


int GameArchive::verify(const std::string& path) {

    std::ifstream ifs(path, std::ifstream::binary);
    if (!ifs)
        return 0;

    auto read_char = [&]() {
        char v;
        if (ifs.read(&v, 1).gcount() != 1)
            throw std::runtime_error("uncomplate move data (read_char)");
        return v;
    };

    auto read_ushort = [&]() {
        unsigned short v;
        if (ifs.read((char*)&v, 2).gcount() != 2)
            throw std::runtime_error("uncomplate move data (read_ushort)");
        return v;
    };

    auto read_int = [&]() {
        int v;
        if (ifs.read((char*)&v, 4).gcount() != 4)
            throw std::runtime_error("uncomplate move data (read_int)");
        return v;
    };

    int read_moves = 0;

    auto type = read_char();
    if (type != 'G') throw std::runtime_error("bad train data signature");

    int boardsize = (int)read_char();
    std::cerr << "loading board size: " << boardsize << std::endl;
    
    while (true) {
        char c;
        if (ifs.read(&c, 1).gcount() == 0)
            break;

        if (c != 'g') throw std::runtime_error("bad move signature");
        read_int();
        
        int result = (int) read_char();
        auto steps = read_ushort();

        if (result !=0 && result !=1 && result != -1)
            throw std::runtime_error("bad game result");

        int player = 1;

        std::vector<int> board(boardsize*boardsize, 0);

        for (auto step = 0; step < steps; step++) {

            auto move_val = read_ushort();
            int pos = (int)(move_val&0x1ff);

            if (pos > boardsize*boardsize) {
                throw std::runtime_error("invalid move pos: " + std::to_string(pos));
            }

            // place the stone 
            if (pos < boardsize*boardsize) {
                if (board[pos] != 0)
                    throw std::runtime_error("board not empty on : " + std::to_string(pos));

                board[pos] = player;
            }

            if (move_val & 0x8000) {
                // has leadding remove stones
                auto count = read_ushort();
                for (int i=0; i<count; i++) {
                    // remove stone 
                    auto pos = read_ushort();
                    if (pos >= boardsize*boardsize) {
                        throw std::runtime_error("invalid removal pos: " + std::to_string(pos));
                    }

                    if (board[pos] != -player)
                        throw std::runtime_error("unexpect removal on : " + std::to_string(pos));

                    board[pos] = 0;
                }
            }

            if (move_val & 0x4000) {
                // has leadding distributions
                int prob_size = boardsize*boardsize + 1;
                std::vector<float> probs(prob_size);
                if (ifs.read((char*)&probs[0], prob_size*sizeof(float)).gcount() != prob_size*sizeof(float))
                    throw std::runtime_error("uncomplate move data (read disttribution)");
            }

            player = -player;
        }

        read_moves += steps;
    }
    
    ifs.close();
    return read_moves;
}
