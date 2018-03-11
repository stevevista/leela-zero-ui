#include "gtp_agent.h"
#include <iostream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <cassert>

inline void trim(std::string &ss)   
{   
	auto p=std::find_if(ss.rbegin(),ss.rend(), std::not1(std::ptr_fun(::isspace)));   
    ss.erase(p.base(),ss.end());  
	auto p2 = std::find_if(ss.begin(),ss.end(), std::not1(std::ptr_fun(::isspace)));   
	ss.erase(ss.begin(), p2);
} 

inline vector<string> split_str(const string& name, char delim, bool strip=true) {

    std::stringstream ss(name);
    std::string item;
    std::vector<std::string> names;
    while (std::getline(ss, item, delim)) {
        names.push_back(item);
    }

    if (strip) {
        if (names.size() && names.front().empty())
            names.erase(names.begin());

        if (names.size() && names.back().empty())
            names.erase(names.end()-1); 
    }

    return names;
}



static string move_to_text(int move, const int board_size) {

    std::ostringstream result;

    int column = move % board_size;
    int row = move / board_size;

    assert(move == GtpState::pass_move || move == GtpState::resign_move || (row >= 0 && row < board_size));
    assert(move == GtpState::pass_move || move == GtpState::resign_move || (column >= 0 && column < board_size));

    if (move >= 0) {
        result << static_cast<char>(column < 8 ? 'A' + column : 'A' + column + 1);
        result << (row + 1);
    } else if (move == GtpState::pass_move) {
        result << "pass";
    } else if (move == GtpState::resign_move) {
        result << "resign";
    } else {
        result << "error";
    }

    return result.str();
}

string GtpState::move_to_text(int move) const {
    return ::move_to_text(move, board_size_);
}

int GtpState::text_to_move(const string& vertex) const {

    if (vertex == "pass" || vertex == "PASS") return pass_move;
    else if (vertex == "resign" || vertex == "RESIGN") return resign_move;

    if (vertex.size() < 2) return invalid_move;
    if (!std::isalpha(vertex[0])) return invalid_move;
    if (!std::isdigit(vertex[1])) return invalid_move;
    if (vertex[0] == 'i') return invalid_move;

    int column, row;
    if (vertex[0] >= 'A' && vertex[0] <= 'Z') {
        if (vertex[0] < 'I') {
            column = vertex[0] - 'A';
        } else {
            column = (vertex[0] - 'A')-1;
        }
    } else {
        if (vertex[0] < 'i') {
            column = vertex[0] - 'a';
        } else {
            column = (vertex[0] - 'a')-1;
        }
    }

    std::string rowstring(vertex);
    rowstring.erase(0, 1);
    std::istringstream parsestream(rowstring);

    parsestream >> row;
    row--;

    if (row >= board_size_ || column >= board_size_) {
        return invalid_move;
    }

    auto move = row * board_size_ + column;
    return move;
}

string GtpState::pending_command() {
    command_t cmd;
    command_queue_.try_peek(cmd);
    return cmd.cmd;
}

bool GtpState::playing_pending() {
    auto pending = pending_command();
    return (pending.find("genmove ") == 0 || pending.find("play ") == 0);
}

void GtpState::clean_command_queue() {

    command_t cmd;
    while (command_queue_.try_pop(cmd)) {
        if (cmd.handler)
            cmd.handler(false, "not active");
    }
}

void GtpState::clean_board() {
    handicaps_.clear();
    history_moves_.clear();
}

void GtpState::clean_up() {

    clean_command_queue();
    board_size_ = 19;
    clean_board();
}

bool GtpState::next_move_is_black() const {

    return (handicaps_.empty() && history_moves_.empty()) ||
            (history_moves_.size() && !history_moves_.back().is_black);
}

vector<GtpState::move_t> GtpState::get_move_sequence() const {
    vector<move_t> out;
    for (auto pos : handicaps_)
        out.push_back({true, pos});

    out.insert(out.end(), history_moves_.begin(), history_moves_.end());
	return out;
}   


//////////////////////////////////////

void GtpProcess::execute(const string& cmdline, const string& path, const int wait_secs) {

    command_line_ = cmdline;
    path_ = path;

    clean_up();
    support_commands_.clear();
    ready_ = false;
    ready_query_made_ = false;
    recvbuffer_.clear(); 

    process_ = make_shared<Process>(command_line_, path_, [this](const char *bytes, size_t n) {
        
        if (onOutput) {
            // skip ready query commamd response
            if (ready_query_made_)
                onOutput(string(bytes, n));
        }

        if (recvbuffer_.empty() && bytes[0] != '=' && bytes[0] != '?') {
            if (onUnexpectOutput)
                onUnexpectOutput(string(bytes, n));

            return;
        }

        // there may be multiple response at same
        int cr = 0;
        if (recvbuffer_.size() && recvbuffer_.back() == '\n')
            cr++;

        for (size_t i=0; i<n; i++) {
            if (bytes[i] == '\n') cr++;
            else cr = 0;

            
            if (recvbuffer_.empty() && bytes[i] == '\n') {
                // skip multiple space 
            }
            else
                recvbuffer_ += bytes[i];

            if (cr == 2) {
                cr = 0;
                auto line = recvbuffer_;
                recvbuffer_ = "";

                // new response
                int id = -1;
                auto ws_pos = line.find(' ');
                if (std::isdigit(line[1]))
                    id = stoi(line.substr(1, ws_pos));

                auto rsp = line.substr(ws_pos+1);
                trim(rsp);

                command_t cmd;
                command_queue_.try_pop(cmd);
                bool success = line[0] == '=';
                onGtpResult(id, success, cmd.cmd, rsp);

                if (cmd.handler)
                    cmd.handler(success, rsp);
            }
        }

    }, [this](const char *bytes, size_t n) {
        if (onStderr)
            onStderr(string(bytes, n));
    }, true);


    send_command("protocol_version", [this](bool ok, const string& rsp) {
        if (!ok) {
            kill();
            return;
        }
        protocol_version_ = rsp;
    });
    send_command("name", [this](bool ok, const string& rsp) {
        if (!ok) {
            kill();
            return;
        }
        name_ = rsp;
    });
    send_command("version", [this](bool ok, const string& rsp) {
        if (!ok) {
            kill();
            return;
        }
        version_ = rsp;
    });
    send_command("list_commands", [this](bool ok, const string& rsp) {
        if (!ok) {
            kill();
            return;
        }
        // update only at first hit
        ready_query_made_ = true;
        std::lock_guard<std::mutex> lk(mtx_); 
        support_commands_ = split_str(rsp, '\n'); 
        ready_ = true;
        if (onReset)
            onReset();
    });

    if (wait_secs == 0) {
        // no wait
        return;
    }

    int passed = 0;
    while (!ready_ && alive()) {
        this_thread::sleep_for(chrono::seconds(1));
        if (passed++ >= wait_secs && wait_secs > 0)
            break;
    }
}

bool GtpProcess::isReady() {
    return ready_;
}

void GtpProcess::onGtpResult(int, bool success, const string& cmd, const string& rsp) {

    if (success) {
        if (cmd.find("boardsize") == 0) {

            std::istringstream cmdstream(cmd);
            std::string stmp;
            int bdsize;

            cmdstream >> stmp;  // eat boardsize
            cmdstream >> bdsize;
            board_size_ = bdsize;
        }
        else if (cmd.find("clear_board") == 0) {
            clean_board();
            if (onReset)
                onReset();
        }
        else if (cmd.find("play") == 0) {
            std::istringstream cmdstream(cmd);
            std::string tmp;
            std::string color, vertex;

            cmdstream >> tmp;   //eat play
            cmdstream >> color;
            cmdstream >> vertex;

            bool is_black = color[0] == 'b';
            auto pos = text_to_move(vertex);
            history_moves_.push_back({is_black, pos});
        }
        else if (cmd.find("genmove") == 0 || cmd.find("kgs-genmove_cleanup") == 0) {
            std::istringstream cmdstream(cmd);
            std::string tmp;
            std::string color;

            cmdstream >> tmp;   //eat genmove
            cmdstream >> color;

            bool is_black = color[0] == 'b';
            auto pos = text_to_move(rsp);
            history_moves_.push_back({is_black, pos});
        }
        else if (cmd.find("undo") == 0) {
            if (history_moves_.size())
                history_moves_.erase(history_moves_.end()-1);
        }
        else if (cmd.find("handicap") != string::npos) {

            // D3 D4 ...
            std::istringstream cmdstream(rsp);
            do {
                std::string vertex;

                cmdstream >> vertex;

                if (!cmdstream.fail()) {
                    auto pos = text_to_move(vertex);
                    if (pos == invalid_move)
                        break;
                    handicaps_.push_back(pos);
                }
            } while (!cmdstream.fail());
        }
    }
}

bool GtpProcess::alive() {
    int exit_status;
    return process_ && !process_->try_get_exit_status(exit_status);
}

bool GtpProcess::support(const string& cmd) {
    std::lock_guard<std::mutex> lk(mtx_);  
    return find(support_commands_.begin(), support_commands_.end(), cmd) != support_commands_.end();   
}


void GtpProcess::send_command(const string& cmd, function<void(bool, const string&)> handler) {

    if (!alive()) {
        if (handler) {
            handler(false, "not active");
        }
        return;
    }

    {
        std::lock_guard<std::mutex> lk(mtx_);  
        command_queue_.push({cmd, handler});
        process_->write(cmd+"\n");
    }

    if (onInput)
        onInput(cmd);
}

void GtpProcess::kill() {

    clean_command_queue();
    if (alive()) 
        process_->kill();
}


bool GtpProcess::restore(int secs) {

    if (!alive()) {

        auto handicaps = handicaps_;
        auto history_moves = history_moves_;
        auto bdsize = board_size_;

        execute(command_line_, path_, secs);

        if (!isReady())
            return false;

        send_command("boardsize " + to_string(bdsize));

        for (auto pos : handicaps) {
            send_command("set_free_handicap " + ::move_to_text(pos, bdsize));
        }

        for (auto& m : history_moves)
            send_command("play " + string(m.is_black? "b " :"w ") + ::move_to_text(m.pos, bdsize));
    }

    return true;
}

string GtpProcess::version() const {
    return version_;
}
