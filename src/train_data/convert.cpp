#include "convert.hpp"
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

    return true;
}

void GameArchive::Writer::start_game() {

    buffer_.clear();
    move_count_ = 0;

    buffer_.emplace_back('g');

    buffer_.emplace_back('x'); // palceholder for result

    buffer_.emplace_back('x'); // palceholder for count
    buffer_.emplace_back('x');
}


int GameArchive::Writer::end_game(int result) {

    buffer_[1] = (char)result;
    std::memcpy(&buffer_[2], &move_count_, 2);
    ofs_.write(&buffer_[0], buffer_.size());
    return buffer_.size();
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


void GameArchive::Writer::add_move(const int move, const std::vector<int>& removes, const std::vector<float>& probs, bool valid) {

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

    move_count_++;
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

    GoBoard b(expect_boardsize);
    int color = 1;

    writer.start_game();

    for (auto idx : tree_moves) {

        if (!b.valid_move(idx))
            return;

        std::vector<int> removed;
        b.update_board(color, idx, removed);

        writer.add_move(idx, removed, {});
        color = -color;
    }

    int move_count = tree_moves.size();
    int wrt_size = writer.end_game(result);
    
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

GameArchive::GameArchive(const int bsize)
: boardsize_(bsize)
{
    data_index = 0;
}

int GameArchive::load(const std::string& path, bool append) {

    if (!append) {
        games_.clear();
        entries_.clear();
    }

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

    int read_moves = 0;

    auto type = read_char();
    if (type != 'G') throw std::runtime_error("bad train data signature");

    boardsize_ = (int)read_char();
    std::cerr << "loading board size: " << boardsize_ << std::endl;
    
    while (true) {
        char c;
        if (ifs.read(&c, 1).gcount() == 0)
            break;

        if (c != 'g') throw std::runtime_error("bad move signature");

        int result = (int) read_char();
        auto steps = read_ushort();

        if (result !=0 && result !=1 && result != -1)
            throw std::runtime_error("bad game result");

        game_t game;
        game.result = result;
        int game_index = games_.size();

        for (auto step = 0; step < steps; step++) {

            move_t move;
            move.pos = read_ushort();


            if ((int)(move.pos&0x1ff) > boardsize_*boardsize_) {
                throw std::runtime_error("invalid move pos: " + std::to_string(move.pos));
            }

            if (move.pos & 0x8000) {
                // has leadding remove stones
                auto count = read_ushort();
                for (int i=0; i<count; i++)
                    move.removes.emplace_back(read_ushort());
            }

            if (move.pos & 0x4000) {
                // has leadding distributions
                int prob_size = boardsize_*boardsize_ + 1;
                move.probs.resize(prob_size);
                if (ifs.read((char*)&move.probs[0], prob_size*sizeof(float)).gcount() != prob_size*sizeof(float))
                    throw std::runtime_error("uncomplate move data (read disttribution)");
            }

            game.seqs.emplace_back(move);
            
            if (!(move.pos & 0x2000)) {
                entries_.push_back({game_index, step});
            }
        }

        games_.push_back(std::move(game));
        read_moves += steps;
    }
    
    ifs.close();

    shuffle();
    return read_moves;
}


void GameArchive::shuffle() {
    std::random_shuffle(entries_.begin(), entries_.end());
}


vector<MoveData> GameArchive::next_batch(int count, const int input_history, bool& rewinded) {
    
    vector<MoveData> out;
    rewinded = false;

    if (total_moves() == 0)
        throw std::runtime_error("empty move archive");

    while (out.size() < count) {

        if (data_index >= total_moves()) {
            data_index = 0;
            rewinded = true;
        }
    
        MoveData data;
        extract_move(data_index++, input_history, data);
        out.emplace_back(data);
    }
    
    return std::move(out);
}

    
void GameArchive::extract_move(const int index, const int input_history, MoveData& out) {

    if (index < 0 || index >= entries_.size())
        throw std::runtime_error("extract_move index error");

    const int input_channels = input_history*2 + 2;

    auto ind = entries_[index];

    const int steps = ind.second;
    const auto& game = games_[ind.first];
    const auto& game_seq = game.seqs;
    int result = game.result;

    const bool to_move_is_black = (steps%2 == 0);
    
    
    out.result = to_move_is_black ? result : -result;

    // replay board
    BoardPlane blacks;
    BoardPlane whites;

    out.input.resize(input_channels);

    blacks = false;
    whites = false;
    for (auto& l : out.input)
        l = false;

    bool black_player = true;
    for (int step = 0; step < steps; step++) {
        
        auto move = game_seq[step].pos;
        auto pos = move & 0x1ff;

        if (pos > boardsize_*boardsize_)
            throw std::runtime_error("bad move pos " + std::to_string(pos));

        auto& my_board = black_player ? blacks : whites;
        auto& op_board = black_player ? whites : blacks;

        if (pos == boardsize_*boardsize_) {
            // pass
        } else {
            my_board[pos] = true;
            if (game_seq[step].removes.size()) {
                for (auto rmpos : game_seq[step].removes) {
                    if (rmpos < 0 || rmpos >= boardsize_*boardsize_)
                        throw std::runtime_error("bad rm move pos " + std::to_string(rmpos));
                    op_board[rmpos] = false;
                }
            }
        }

        // copy history
		int h = steps - step - 1;
		if (h < input_history ) {
            // collect white, black occupation planes
            out.input[h] = to_move_is_black ? blacks : whites;
            out.input[input_history + h] = to_move_is_black ? whites : blacks;
		}

        black_player = !black_player;
    }


    out.probs.resize(boardsize_*boardsize_+1, 0);

    if (game_seq[steps].probs.size()) {
        std::copy(game_seq[steps].probs.begin(), game_seq[steps].probs.end(), out.probs.begin());
    } else {
        const int cur_move = game_seq[steps].pos & 0x1ff;
        out.probs[cur_move] = 1;
    }

    if (to_move_is_black)
        out.input[input_channels - 2].set();
    else
        out.input[input_channels - 1].set();
}
