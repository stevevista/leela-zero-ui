#include "../FastState.h"
#include <algorithm>
#include <unordered_set>

using namespace std;

class QuickBoard : public FastBoard {
public:
    explicit QuickBoard() = default;
    explicit QuickBoard(const FastState& rhs) {
        // Copy in fields from base class.
        *(static_cast<FastBoard*>(this)) = rhs.board;
        m_komove = rhs.m_komove;
    }

    bool isLadder(int v_atr, int depth) const;
    bool IsWastefulEscape(int color, int v) const;

    int find_lib_atr(int vtx) const;
    unordered_set<int> find_libs(int vtx) const;

    void update_board(const int color, const int i);
    int remove_string(int i);

    int m_komove;
};

int QuickBoard::remove_string(int i) {
    int pos = i;
    int removed = 0;
    int color = m_square[i];

    do {
        m_square[pos] = EMPTY;
        m_parent[pos] = MAXSQ;

        remove_neighbour(pos, color);

        m_empty_idx[pos]      = m_empty_cnt;
        m_empty[m_empty_cnt]  = pos;
        m_empty_cnt++;

        removed++;
        pos = m_next[pos];
    } while (pos != i);

    return removed;
}

void QuickBoard::update_board(const int color, const int i) {

    m_square[i] = (square_t)color;
    m_next[i] = i;
    m_parent[i] = i;
    m_libs[i] = count_pliberties(i);
    m_stones[i] = 1;

    /* update neighbor liberties (they all lose 1) */
    add_neighbour(i, color);

    /* did we play into an opponent eye? */
    auto eyeplay = (m_neighbours[i] & s_eyemask[!color]);

    auto captured_stones = 0;
    int captured_sq;

    for (int k = 0; k < 4; k++) {
        int ai = i + m_dirs[k];

        if (m_square[ai] == !color) {
            if (m_libs[m_parent[ai]] <= 0) {
                int this_captured = remove_string(ai);
                captured_sq = ai;
                captured_stones += this_captured;
            }
        } else if (m_square[ai] == color) {
            int ip = m_parent[i];
            int aip = m_parent[ai];

            if (ip != aip) {
                if (m_stones[ip] >= m_stones[aip]) {
                    merge_strings(ip, aip);
                } else {
                    merge_strings(aip, ip);
                }
            }
        }
    }

    m_prisoners[color] += captured_stones;


    /* move last vertex in list to our position */
    auto lastvertex = m_empty[--m_empty_cnt];
    m_empty_idx[lastvertex] = m_empty_idx[i];
    m_empty[m_empty_idx[i]] = lastvertex;

    /* check whether we still live (i.e. detect suicide) */
    if (m_libs[m_parent[i]] == 0) {
        remove_string(i);
    }

    /* check for possible simple ko */
    if (captured_stones == 1 && eyeplay) {
        m_komove = captured_sq;
    }
    else 
        m_komove = 0;
}


int QuickBoard::find_lib_atr(int vtx) const {

    /* loop over stones, update parents */
    int pos = vtx;

    do {
        // check if this stone has a liberty
        for (int k = 0; k < 4; k++) {
            int ai = pos + m_dirs[k];
            // for each liberty, check if it is not shared
            if (m_square[ai] == EMPTY) {
                return ai;
            }
        }
        pos = m_next[pos];
    } while (pos != vtx);

    return -1;
}

unordered_set<int> QuickBoard::find_libs(int vtx) const {

    unordered_set<int> out;
    /* loop over stones, update parents */
    int pos = vtx;

    do {
        // check if this stone has a liberty
        for (int k = 0; k < 4; k++) {
            int ai = pos + m_dirs[k];
            // for each liberty, check if it is not shared
            if (m_square[ai] == EMPTY) {
                out.insert(ai);
            }
        }
        pos = m_next[pos];
    } while (pos != vtx);

    return out;
}

// Return whether the Ren including v_atr is captured when it escapes.
bool QuickBoard::isLadder(int v_atr, int depth) const {

    if(depth >= 128) return false;

    int v_esc = find_lib_atr(v_atr);
    const int color = m_square[v_atr];

    //    Check whether surrounding stones can be taken.
    std::vector<int> possible_escapes;
    unordered_set<int> visited;

    int pos = v_atr;

    do {
        for (int k = 0; k < 4; k++) {
            int ai = pos + m_dirs[k];
            if (m_square[ai] == !color &&
                m_libs[m_parent[ai]] == 1) {

                if (visited.find(m_parent[ai]) == visited.end()) {
                    visited.insert(m_parent[ai]);
                    auto lib_atr = find_lib_atr(ai);
                    if (lib_atr != m_komove)
                        possible_escapes.push_back(lib_atr);
                }
            }
        }
        pos = m_next[pos];
    } while (pos != v_atr);

    if (v_esc != m_komove &&
        find(possible_escapes.begin(), possible_escapes.end(), v_esc) == possible_escapes.end()) {
        possible_escapes.push_back(v_esc);
    }

    for(auto v_cap: possible_escapes) {

        QuickBoard b(*this);
		b.update_board(color, v_cap);

        if(b.m_libs[b.m_parent[v_atr]] <= 1) {
			continue;
		}

        if(b.m_libs[b.m_parent[v_atr]] > 2){
			// Return false when number of liberty > 2.
			return false;
		}

        auto libs = b.find_libs(v_atr);
        bool is_ladder = false;
		for(auto lib: libs) {
			if(lib != b.m_komove) {
                QuickBoard bb(b);
			    bb.update_board(!color, lib);
				// Recursive search.
				is_ladder |= bb.isLadder(v_atr, depth + 1);
			}
		}
        if(!is_ladder) return false; // Successfully escape.
    }
    return true;
}


bool QuickBoard::IsWastefulEscape(int color, int v) const {

    std::array<int, 4> nbr_pars;
    int nbr_par_cnt = 0;

    //    Check neighboring 4 positions.
    for (int k = 0; k < 4; k++) {
        int ai = v + m_dirs[k];
        if (m_square[ai] == color && m_libs[m_parent[ai]] == 1) {
            bool found = false;
            for (int i = 0; i < nbr_par_cnt; i++) {
                if (nbr_pars[i] == m_parent[ai]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                nbr_pars[nbr_par_cnt++] = m_parent[ai];
                if (isLadder(ai, 0))
                    return true;
            }
        }
    }
    return false;
}

bool IsWastefulEscape(const FastState& state, int color, int v) {
    
    if (v == state.m_komove ||
        v == FastBoard::PASS ||
        v == FastBoard::RESIGN || 
        state.board.get_square(v) != FastBoard::EMPTY ||
        state.board.count_pliberties(v) > 2)
        return false;

    QuickBoard b(state);
    return b.IsWastefulEscape(color, v);
}
