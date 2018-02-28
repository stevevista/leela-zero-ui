#include "gtp_agent.h"
#include <iostream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <sstream>
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


GtpAgent::GtpAgent(const string& cmdline, const string& path)
{
    if (!cmdline.empty())
        execute(cmdline, path);
}

void GtpAgent::execute(const string& cmdline, const string& path) {

    command_line_ = cmdline;
    path_ = path;

    clean_command_queue();
    support_commands_.clear();
    ready_ = false;
    ready_query_made_ = false; 
    board_size_ = 19;
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

    }, nullptr, true);


    send_command("protocol_version");
    send_command("name");
    send_command("version");
    send_command("list_commands");
}

bool GtpAgent::isReady() {
    return ready_;
}

void GtpAgent::onGtpResult(int, bool success, const string& cmd, const string& rsp) {

    if (!ready_query_made_) {

        if (!success) {
            kill();
            clean_command_queue();
            return;
        }

        if (cmd == "list_commands") {

            // update only at first hit
            ready_query_made_ = true;
            std::lock_guard<std::mutex> lk(mtx_); 
            support_commands_ = split_str(rsp, '\n'); 
            ready_ = true;
        }
        else if (cmd == "protocol_version")
            protocol_version_ = rsp;
        else if (cmd == "version")
            version_ = rsp;
        else if (cmd == "name")
            name_ = rsp;
    
        return;
    }

    
}

bool GtpAgent::alive() {
    int exit_status;
    return !process_->try_get_exit_status(exit_status);
}

bool GtpAgent::support(const string& cmd) {
    std::lock_guard<std::mutex> lk(mtx_);  
    return find(support_commands_.begin(), support_commands_.end(), cmd) != support_commands_.end();   
}


void GtpAgent::clean_command_queue() {

    command_t cmd;
    while (command_queue_.try_pop(cmd)) {
        if (cmd.handler)
            cmd.handler(false, "not active");
    }
}

void GtpAgent::send_command(const string& cmd, function<void(bool, const string&)> handler) {

    if (!alive()) {
        if (handler) {
            handler(false, "not active");
        }
        return;
    }

    std::lock_guard<std::mutex> lk(mtx_);  
    command_queue_.push({cmd, handler});
    process_->write(cmd+"\n");
}

string GtpAgent::send_command_sync(const string& cmd, bool& success) {

    std::condition_variable cond;
    string ret;
    send_command(cmd, [&success, &cond, &ret](bool ok, const string& out) {

        success = ok;
        ret = out;
        cond.notify_one();
    });

    std::mutex m;
    std::unique_lock<std::mutex> lk(m);
    cond.wait(lk);  
    return ret;
}

void GtpAgent::kill() {
    if (alive()) 
        process_->kill();
}


static int text_to_move(const string& vertex, const int board_size) {

    if (vertex == "pass" || vertex == "PASS") return GtpAgent::pass_move;
    else if (vertex == "resign" || vertex == "RESIGN") return GtpAgent::resign_move;

    if (vertex.size() < 2) return GtpAgent::invalid_move;
    if (!std::isalpha(vertex[0])) return GtpAgent::invalid_move;
    if (!std::isdigit(vertex[1])) return GtpAgent::invalid_move;
    if (vertex[0] == 'i') return GtpAgent::invalid_move;

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

    if (row >= board_size || column >= board_size) {
        return GtpAgent::invalid_move;
    }

    auto move = row * board_size + column;
    return move;
}

static string move_to_text(int move, const int board_size) {

    std::ostringstream result;

    int column = move % board_size;
    int row = move / board_size;

    assert(move == GtpAgent::pass_move || move == GtpAgent::resign_move || (row >= 0 && row < board_size));
    assert(move == GtpAgent::pass_move || move == GtpAgent::resign_move || (column >= 0 && column < board_size));

    if (move >= 0) {
        result << static_cast<char>(column < 8 ? 'A' + column : 'A' + column + 1);
        result << (row + 1);
    } else if (move == GtpAgent::pass_move) {
        result << "pass";
    } else if (move == GtpAgent::resign_move) {
        result << "resign";
    } else {
        result << "error";
    }

    return result.str();
}

void GtpAgent::generate_move(bool black_move, function<void(int)> handler) {

    send_command(black_move ? "genmove b" : "genmove w", [handler, this](bool success, const string& rsp) {

        if (!success) handler(cmd_fail);
        else {
            handler(text_to_move(rsp, board_size_));
        }

    });
}


void GtpAgent::play(bool black_move, int pos, function<void(int)> handler) {

    auto mtext = move_to_text(pos, board_size_);
    if (mtext.empty()) {
        if (handler) handler(GtpAgent::invalid_move);
        return;
    }

    string cmd = black_move ? "play b " : "play w ";
    cmd += mtext;

    send_command(cmd, [handler, this](bool success, const string&) {

        if (handler) handler(success ? 0 : cmd_fail);

    });
}

void GtpAgent::undo(function<void(int)> handler) {

    send_command("undo", [handler, this](bool success, const string&) {

        if (handler) handler(success ? 0 : cmd_fail);

    });
}

void GtpAgent::quit() {
    send_command("quit");
}   

GameAdvisor::GameAdvisor(const string& cmdline, const string& path)
: GtpAgent(cmdline, path)
{}

void GameAdvisor::think(bool black_move) {

    events_.push("think;");

    generate_move(black_move, [black_move, this](int move){
        if (!pending_reset_) {
            string player = black_move ? "b;" : "w;";
            events_.push("think_end;");
            if (move == pass_move)
                events_.push("pass;" + player);
            else if (move == resign_move)
                events_.push("resign;" + player);
            else if (move >= 0) {
                std::stringstream ss;
                ss << "move;";
                ss << player;
                ss << move << ";";
                events_.push(ss.str());
            } else {
                throw std::runtime_error("unexpected mvoe " + to_string(move));
            }

            {
                std::lock_guard<std::mutex> lk(hmtx_);  
                if (history_.size() && last_player_ == black_move) {
                    int last_move = history_.back();
                    if (last_move != move) {
                        // correct the actual move
                        // duplicated move
                        undo();
                        undo();
        
                        play(black_move, last_move, [this](int ret) {
                            if (ret != 0)
                                throw std::runtime_error("unexpected play result " + to_string(ret));
                        });

                    } else {
                        // as expected
                        // no need do anything
                    }
                }
                else {
                    commit_pending_ = true;
                    last_player_ = black_move;
                    history_.push_back(move);
                }
            }
        }
    });
}

void GameAdvisor::pop_events() {

    string ev;
	while (events_.try_pop(ev)) {
        auto params = split_str(ev, ';');
        if (params[0] == "move") {
            bool black_move = params[1] == "b";
            int move = stoi(params[2]);
            if (onThinkMove)
				onThinkMove(black_move, move);
        }
    }
}

void GameAdvisor::place(bool black_move, int pos) {

    if (commit_pending_) {

        commit_pending_ = false;

        if (black_move == last_player_) {
            if (pos == history_.back()) {
                // as expected 
                return;
            } else {
                undo();
                std::lock_guard<std::mutex> lk(hmtx_);  
                history_.erase(history_.end()-1);
            }
        } else {
            undo(); // not my turn
            std::lock_guard<std::mutex> lk(hmtx_);  
            history_.erase(history_.end()-1);
        }
    }

    {
        std::lock_guard<std::mutex> lk(hmtx_);  
        last_player_ = black_move;
        history_.push_back(pos);
    }

    play(black_move, pos, [this](int ret) {
        if (ret != 0)
            throw std::runtime_error("unexpected play result " + to_string(ret));
    });
}


void GameAdvisor::reset() {

    send_command("clear_board", [this](bool success, const string&) {

        if (!success) throw std::runtime_error("unexpected clear_board result ");
        
        reset_vars();

        events_.push("reset;");
    });
    pending_reset_ = true;
}

bool GameAdvisor::restore(int secs) {

    if (!alive()) {
        execute(command_line_, path_);

        if (!wait_till_ready(secs))
            return false;

        bool player = true;
        auto steps = history_;
        reset_vars();

        for (auto m : steps) {
            place(player, m);
            player = !player;
        }

    }

    return true;
}

void GameAdvisor::reset_vars() {
    while (!events_.empty()) events_.try_pop();
    history_.clear();
    pending_reset_ = false;
    commit_pending_ = false;
}

bool GameAdvisor::wait_till_ready(int secs) {

    int passed = 0;
    while (!isReady()) {
        this_thread::sleep_for(chrono::seconds(1));
        if (passed++ >= secs)
            break;
    }
    return isReady();
}
