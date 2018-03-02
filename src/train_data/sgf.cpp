#include "sgf.hpp"
#include <iostream>
#include <fstream>
#include <cassert>
#include <cctype>

static std::string parse_property_name(std::istringstream & strm);
static bool parse_property_value(std::istringstream & strm, std::string & result);

inline bool is7bit(int c) {
    return c >= 0 && c <= 127;
}

static int string_to_move(const std::string& movestring, int bsize) {

    if (movestring.size() == 0) {
        return bsize*bsize;
    }

    if (movestring == "tt") {
        return bsize*bsize;
    }

    char c1 = movestring[0];
    char c2 = movestring[1];

    int cc1;
    int cc2;

    if (c1 >= 'A' && c1 <= 'Z') {
        cc1 = 26 + c1 - 'A';
    } else {
        cc1 = c1 - 'a';
    }
    if (c2 >= 'A' && c2 <= 'Z') {
        cc2 = bsize - 26 - (c2 - 'A') - 1;
    } else {
        cc2 = bsize - (c2 - 'a') - 1;
    }

    // catch illegal SGF
    if (cc1 < 0 || cc1 >= bsize
        || cc2 < 0 || cc2 >= bsize) {
        std::cout << movestring << std::endl;
        throw std::runtime_error("Illegal SGF move");
    }

    return cc2*bsize + cc1;
}

std::vector<int> seq_to_moves(const std::string& seqs, const int boardsize) {
    
    char next_player = 'B';

    std::vector<int> moves;
        
    int i = 0;
    
    while (i < seqs.size()) {
    
        char c = seqs[i++];
        if (c != ';') 
            break;
        c = seqs[i++];
        if (c != next_player) {
            moves.clear();
            break;
        }
    
        c = seqs[i++];
        if (c != '[')
            break;

        auto close_pos = seqs.find(']', i);
        if (close_pos == std::string::npos)
            break;

        auto n = close_pos - i;
        auto pos = string_to_move(seqs.substr(i, n), boardsize);
        i += n+1;

        if (next_player == 'B') next_player = 'W';
        else next_player = 'B';

        moves.push_back(pos);
    }
    
    return moves;
}

void SGFParser::parse(std::istringstream & strm,
        int& boardsize,
        float& komi,
        float& result,
        std::vector<int>& moves) {

    bool splitpoint = false;
    int level = -1;

    boardsize = 19;
    komi = 7.5;
    result = 0;

    char next_player = 'B';

    char c;
    while (strm >> c) {
        if (strm.fail()) {
            return;
        }

        if (std::isspace(c)) {
            continue;
        }

        // parse a property
        if (std::isalpha(c) && std::isupper(c)) {
            strm.unget();

            std::string propname = parse_property_name(strm);
            bool success;

            do {
                std::string propval;
                success = parse_property_value(strm, propval);
                if (success) {

                    bool has_main_prop = true;
                    bool valid = true;

                    if (propname == "GM") {
                        if (propval != "1") {
                            // SGF Game is not a Go game
                            valid = false;
                        }
                    } else if (propname == "SZ") {
                        std::istringstream strm(propval);
                        strm >> boardsize;
                    } else if (propname == "AB" || propname == "AW") {
                        valid = false; // handicaps
                    } else if (propname == "PL") {
                        if (propval != "B")
                            valid = false;
                    } else if (propname == "HA") {
                        std::istringstream strm(propval);
                        float handicap;
                        strm >> handicap;
                        if (handicap > 0)
                            valid = false;

                    } else if (propname == "KM") {
                        std::istringstream strm(propval);
                        strm >> komi;
                    } else if (propname == "RE") {
                        if (propval.find("B+") == 0 || propval.find("W+") == 0) {
                            auto reason = propval.substr(2);
                            if (reason[0] != 'R') {
                                result = std::atof(reason.c_str());
                            } else 
                                result = 1000;

                            if (propval.find("W+") == 0)
                                result = -result;
                        }
                    } else {
                        has_main_prop = false;
                    }

                    if (!valid) {
                        moves.clear();
                        return;
                    }

                    if (has_main_prop && level < 0)
                        level = 0;

                    if (propname == "B" || propname == "W") {
                        if (propname[0] != next_player) {
                            moves.clear();
                            return;
                        }

                        auto pos = string_to_move(propval, boardsize);

                        if (next_player == 'B') next_player = 'W';
                        else next_player = 'B';

                        moves.push_back(pos);
                    }
                }
            } while (success);

            continue;
        }

        if (c == '(') {
            // eat first ;
            char cc;
            do {
                strm >> cc;
            } while (std::isspace(cc));
            if (cc != ';') {
                strm.unget();
            }
            // start a variation here
            splitpoint = true;

            if (level < 0)
                parse(strm, boardsize, komi, result, moves);
            else {
                int dump_boardsize;
                float dump_komi;
                float dump_result;
                std::vector<int> dump_moves;
                parse(strm, dump_boardsize, dump_komi, dump_result, dump_moves);
            }

        } else if (c == ')') {
            // variation ends, go back
            // if the variation didn't start here, then
            // push the "variation ends" mark back
            // and try again one level up the tree
            if (!splitpoint) {
                strm.unget();
                return;
            } else {
                splitpoint = false;
                continue;
            }
        } else if (c == ';') {
            // new node
            continue;
        }
    }
}

std::string parse_property_name(std::istringstream & strm) {
    std::string result;

    char c;
    while (strm >> c) {
        // SGF property names are guaranteed to be uppercase,
        // except that some implementations like IGS are retarded
        // and don't folow the spec. So allow both upper/lowercase.
        if (!std::isupper(c) && !std::islower(c)) {
            strm.unget();
            break;
        } else {
            result.push_back(c);
        }
    }

    return result;
}

bool parse_property_value(std::istringstream & strm,
                                     std::string & result) {
    strm >> std::noskipws;

    char c;
    while (strm >> c) {
        if (!std::isspace(c)) {
            strm.unget();
            break;
        }
    }

    strm >> c;

    if (c != '[') {
        strm.unget();
        return false;
    }

    while (strm >> c) {
        if (c == ']') {
            break;
        } else if (c == '\\') {
            strm >> c;
        }
        result.push_back(c);
    }

    strm >> std::skipws;

    return true;
}


static std::vector<std::string> chop_stream(std::istream& ins,
                                                size_t stopat);

std::vector<std::string> sgf_chop_all(std::string filename,
                                             size_t stopat) {
    std::ifstream ins(filename.c_str(), std::ifstream::binary | std::ifstream::in);

    if (ins.fail()) {
        throw std::runtime_error("Error opening file");
    }

    auto result = chop_stream(ins, stopat);
    ins.close();

    return result;
}


std::vector<std::string> chop_stream(std::istream& ins,
                                                size_t stopat) {
    std::vector<std::string> result;
    std::string gamebuff;

    ins >> std::noskipws;

    int nesting = 0;      // parentheses
    bool intag = false;   // brackets
    int line = 0;
    gamebuff.clear();

    char c;
    while (ins >> c && result.size() <= stopat) {
        if (c == '\n') line++;

        gamebuff.push_back(c);
        if (c == '\\') {
            // read literal char
            ins >> c;
            gamebuff.push_back(c);
            // Skip special char parsing
            continue;
        }

        if (c == '(' && !intag) {
            if (nesting == 0) {
                // eat ; too
                do {
                    ins >> c;
                } while(std::isspace(c) && c != ';');
                gamebuff.clear();
            }
            nesting++;
        } else if (c == ')' && !intag) {
            nesting--;

            if (nesting == 0) {
                result.push_back(gamebuff);
            }
        } else if (c == '[' && !intag) {
            intag = true;
        } else if (c == ']') {
            if (intag == false) {
                std::cerr << "Tag error on line " << line << std::endl;
            }
            intag = false;
        }
    }

    // No game found? Assume closing tag was missing (OGS)
    if (result.size() == 0) {
        result.push_back(gamebuff);
    }

    return result;
}


int SGFParser::count_games_in_file(std::string filename) {
    std::ifstream ins(filename.c_str(), std::ifstream::binary | std::ifstream::in);

    if (ins.fail()) {
        throw std::runtime_error("Error opening file");
    }

    int count = 0;
    int nesting = 0;

    char c;
    while (ins >> c) {
        if (!is7bit(c)) {
            do {
                ins >> c;
            } while (!is7bit(c));
            continue;
        }

        if (c == '\\') {
            // read literal char
            ins >> c;
            // Skip special char parsing
            continue;
        }

        if (c == '(') {
            nesting++;
        } else if (c == ')') {
            nesting--;

            assert(nesting >= 0);

            if (nesting == 0) {
                // one game processed
                count++;
            }
        }
    }

    ins.close();

    return count;
}

