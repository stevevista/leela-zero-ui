#include "board.h"
#include <stdexcept>
#include <cassert>
#include <array>


static std::array<std::vector<int>, 361> NEIGHBORS;
static int init_boardsize = 0;

static void init_board_global(const int bsize) {

    init_boardsize = bsize;
    for (int y=0; y<bsize; y++) {
        for (int x=0; x<bsize; x++) {
            auto& n = NEIGHBORS[y*bsize + x];

            if (y > 0) n.emplace_back((y-1)*bsize + x);
            if (y < bsize-1) n.emplace_back((y+1)*bsize + x);
            if (x > 0) n.emplace_back(y*bsize + x - 1);
            if (x < bsize-1) n.emplace_back(y*bsize + x + 1);
        }
    }
}


GoBoard::GoBoard(const int bsize) {
    reset(bsize);
}

class NeighborVistor {
    int nbr_pars[4];
    int nbr_par_cnt = 0;
public:
    bool visited(int pos) {
        for (int i = 0; i < nbr_par_cnt; i++) {
            if (nbr_pars[i] == pos) {
                return true;
            }
        }
        nbr_pars[nbr_par_cnt++] = pos;
        return false;
    }
};

void GoBoard::reset(int bsize) {

    if (bsize == 0)
        bsize = boardsize;
    
    if (init_boardsize != bsize) {
        init_board_global(bsize);
    }
    boardsize = bsize;

    for (int i = 0; i < boardsize*boardsize; i++) {
        stones[i] = 0;
    }
}

bool GoBoard::update_board(const bool black, const int i) {

    if (i >= boardsize*boardsize || stones[i]) {
        std::cerr << i << std::endl;
        std::cerr << stones[i] << std::endl;
        return false;
    }

    const int color = black ? 1 : -1;
    
    // pass
    if (i < 0)
        return true;

    stones[i] = color;
    stone_next[i] = i;
    group_ids[i] = i;

    int libs = 0;
    NeighborVistor vistor;

    for (auto ai : NEIGHBORS[i]) {

        if (stones[ai] == 0)
            libs++;
        else {
            int g = group_ids[ai];
            if (!vistor.visited(g)) {
                group_libs[g]--;
            }
        }
    }

    group_libs[i] = libs;

    for (auto ai : NEIGHBORS[i]) {

        if (stones[ai] == -color) {
            if (group_libs[group_ids[ai]] == 0) {
                remove_string(ai);
            }
        } else if (stones[ai] == color) {
            int ip = group_ids[i];
            int aip = group_ids[ai];

            if (ip != aip) {
                merge_strings(aip, ip);
            }
        }
    }

    // check whether we still live (i.e. detect suicide)
    if (group_libs[group_ids[i]] == 0) {
        remove_string(group_ids[i]);
    }

    return true;
}

void GoBoard::remove_string(int i) {

    int pos = i;

    do {
        stones[pos]  = 0;

        NeighborVistor vistor;
        
        for (auto ai : NEIGHBORS[pos]) {

            if (!stones[ai]) continue;

            if (!vistor.visited(group_ids[ai])) {
                group_libs[group_ids[ai]]++;
            }
        }

        pos = stone_next[pos];
    } while (pos != i);
}

void GoBoard::merge_strings(const int ip, const int aip) {

    /* loop over stones, update parents */
    int newpos = aip;

    do {
        // check if this stone has a liberty
        for (auto ai : NEIGHBORS[newpos]) {

            // for each liberty, check if it is not shared
            if (stones[ai] == 0) {
                // find liberty neighbors
                bool found = false;
                for (auto aai : NEIGHBORS[ai]) {

                    if (!stones[aai]) continue;

                    // friendly string shouldn't be ip
                    // ip can also be an aip that has been marked
                    if (group_ids[aai] == ip) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    group_libs[ip]++;
                }
            }
        }

        group_ids[newpos] = ip;
        newpos = stone_next[newpos];
    } while (newpos != aip);

    /* merge stings */
    int tmp = stone_next[aip];
    stone_next[aip] = stone_next[ip];
    stone_next[ip] = tmp;
}
