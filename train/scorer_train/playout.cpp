#include "playout.h"
#include <iostream>

#define forEach4Nbr(v_origin,v_nbr,block)		\
	int v_nbr;									\
	v_nbr = v_origin + 1;			block;		\
	v_nbr = v_origin - 1;			block;		\
	v_nbr = v_origin + EBSIZE;		block;		\
	v_nbr = v_origin - EBSIZE;		block;

bool japanese_rule = false;

/**
 *
 *  Return the result of the board in the end of the game.
 *  white wins: 0, black wins: 1 or -1.
 *  Before calling this function, both players need to play
 *  until legal moves cease to exist.
 *  Updates game_cnt, stone_cnt and owner_cnt at the same time.
 */
int Win(Board& b, int pl, double komi) {

	double score[2] = {0.0, 0.0};
	std::array<bool, EBVCNT> visited;
	std::fill(visited.begin(), visited.end(), false);
	std::array<bool, EBVCNT> is_stone[2];
	std::fill(is_stone[0].begin(), is_stone[0].end(), false);
	std::fill(is_stone[1].begin(), is_stone[1].end(), false);

	// Check Seki.
	for(int i=0,i_max=b.empty_cnt;i<i_max;++i){
		int v = b.empty[i];
		if(b.IsSeki(v) && !visited[v]){
			// Check whether it is corner bent fours.
			int ren_idx[2] = {0,0};
			forEach4Nbr(v, v_nbr2, {
				if(b.color[v_nbr2] > 1){
					ren_idx[b.color[v_nbr2] - 2] = b.ren_idx[v_nbr2];
				}
			});
			int bnd_pl = -1;
			for(int j=0;j<2;++j){
				if(b.ren[ren_idx[j]].size == 3){
					int v_tmp = ren_idx[j];
					bool is_edge = true;
					bool is_conner = false;

					do{
						is_edge &= (DistEdge(v_tmp) == 1);
						if(!is_edge) break;
						if (	v_tmp == rtoe[0] 					||
								v_tmp == rtoe[BSIZE - 1] 			||
								v_tmp == rtoe[BSIZE * (BSIZE - 1)] 	||
								v_tmp == rtoe[BVCNT - 1]	){

							bool is_not_bnt = false;
							forEach4Nbr(v_tmp, v_nbr1, {
								// If the neighboring stone is an opponnent's one.
								is_not_bnt |= (b.color[v_nbr1] == int(j==0) + 2);
							});

							is_conner = !is_not_bnt;
						}

						v_tmp = b.next_ren_v[v_tmp];
					}while(v_tmp != ren_idx[j]);

					if(is_edge && is_conner){
						// Count all stones as that of the player of the bent fours.
						score[j] += b.ren[ren_idx[0]].size + b.ren[ren_idx[1]].size + 2.0;
						bnd_pl = j;
					}
				}
			}
			// Update visited.
			int64 lib_bit;
			for(int i=0;i<6;++i){
				lib_bit = b.ren[ren_idx[0]].lib_bits[i];
				while(lib_bit != 0){
					int ntz = NTZ(lib_bit);
					int lib = rtoe[ntz + i * 64];
					visited[lib] = true;

					lib_bit ^= (0x1ULL << ntz);
				}
			}

			if(bnd_pl != -1){
				int v_tmp = ren_idx[0];
				do{
					visited[v_tmp] = true;
					is_stone[bnd_pl][v_tmp] = true;

					v_tmp = b.next_ren_v[v_tmp];
				}while(v_tmp != ren_idx[0]);
				v_tmp = ren_idx[1];
				do{
					visited[v_tmp] = true;
					is_stone[bnd_pl][v_tmp] = true;

					v_tmp = b.next_ren_v[v_tmp];
				}while(v_tmp != ren_idx[1]);
			}
		}
	}

	for (auto i: rtoe) {
		int stone_color = b.color[i] - 2;
		if (!visited[i] && stone_color >= 0) {
			visited[i] = true;
			++score[stone_color];
			is_stone[stone_color][i] = true;
			forEach4Nbr(i, v_nbr, {
				if (!visited[v_nbr] && b.color[v_nbr] == 0) {
					visited[v_nbr] = true;
					++score[stone_color];
				}
			});
		}
	}

	// Correction factor of PASS. Add one if the last move is black.
	int pass_corr = b.pass_cnt[0] - b.pass_cnt[1] + int((b.move_cnt%2)!=0);
	double abs_score = score[1] - score[0] - komi - pass_corr * int(japanese_rule);

	// Returns 0 if white wins, 1 if black wins and it's black's turn and else -1.
	return int(abs_score > 0)*(int(pl == 1) - int(pl == 0));

}

/**
 *
 *  Play with 'Last Good Reply' until the end and returns the result.
 *  white wins: 0, black wins: 1 or -1.
 *  Updates game_cnt, stone_cnt and owner_cnt at the same time.
 */
int PlayoutLGR(Board& b, LGR& lgr, double komi)
{

	int next_move;
	int prev_move = VNULL;
	int pl = b.my;
	std::array<int, 4> lgr_seed;
	std::vector<std::array<int, 3>> lgr_rollout_add[2];
	std::array<int, 3> lgr_rollout_seed;
	int update_v[2] = {VNULL, VNULL};
	double update_p[2] = {100, 25};

	while (b.move_cnt <= 720) {
		lgr_seed[0] = b.prev_ptn[0].bf;
		lgr_seed[1] = b.prev_move[b.her];
		lgr_seed[2] = b.prev_ptn[1].bf;
		lgr_seed[3] = b.prev_move[b.my];

		// Forced move if removed stones is Nakade.
		if (b.response_move[0] != VNULL) {
			next_move = b.response_move[0];
			b.PlayLegal(next_move);
		}
		else{
			int v = VNULL;
			if(lgr_seed[1] < PASS && lgr_seed[3] < PASS){
				v = lgr.rollout[b.my][lgr_seed[1]][lgr_seed[3]];
				if(v != VNULL){
					//lgr_rollout
					if(b.prob[b.my][v] != 0){
						b.ReplaceProb(b.my, v, b.prob[b.my][v] * update_p[1]);
						update_v[1] = v;
					}
				}
			}

			next_move = b.SelectMove();

			// update_v
			// Restore probability.
			for(int i=0;i<2;++i){
				if(update_v[i] != VNULL){
					if(b.prob[b.her][update_v[i]] != 0){
						b.ReplaceProb(b.her, update_v[i], b.prob[b.her][update_v[i]] / update_p[i]);
					}
					update_v[i] = VNULL;
				}
			}

		}

		if(lgr_seed[1] < PASS && lgr_seed[3] < PASS && next_move != PASS){
			lgr_rollout_seed[0] = lgr_seed[1];
			lgr_rollout_seed[1] = lgr_seed[3];
			lgr_rollout_seed[2] = next_move;
			lgr_rollout_add[b.her].push_back(lgr_rollout_seed);
		}

		// Break in case of 2 consecutive pass.
		if (next_move==PASS && prev_move==PASS) break;
		prev_move = next_move;
	}

	prev_move = VNULL;
	while (b.move_cnt <= 720) {
		next_move = b.SelectRandomMove();
		if (next_move==PASS && prev_move==PASS) break;
		prev_move = next_move;
	}

	int win = Win(b, pl, komi);
	int win_pl = int(win != 0);
	int lose_pl = int(win == 0);

	for(auto& i:lgr_rollout_add[win_pl]){
		lgr.rollout[win_pl][i[0]][i[1]] = i[2];
	}

	for(auto& i:lgr_rollout_add[lose_pl]){
		if(lgr.rollout[lose_pl][i[0]][i[1]] == i[2]){
			lgr.rollout[lose_pl][i[0]][i[1]] = VNULL;
		}
	}

	// Return the result.
	return win;

}



std::vector<unsigned char> generate_sample() {

	int next_move;
	int prev_move = VNULL;
	Board b;

	// 46 bytes to presents a 361 points
	// magic 'v' + 16 history + to_move(1 for white) + result
	std::vector<unsigned char> ft(1 + 46*16 + 1 + 1, 0);

	ft[0] = (unsigned char)'v';

	const int steps = int(mt_double(mt_32) * 300);
	const bool to_move_is_black = (steps%2 == 0);

	while (b.move_cnt < steps) {
		next_move = b.SelectMove();

		int h = steps - b.move_cnt;
		if (h < 8 ) {
			unsigned char* blacks, *whites;
			if (to_move_is_black) {
				blacks = &ft[1+46*h];
				whites = &ft[1+46*(h+8)];
			} else {
				blacks = &ft[1+46*(h+8)];
				whites = &ft[1+46*h];
			}
			for (int i=0; i<361; i++) {
				auto vtx = rtoe[i];
				auto p = i/8;
				auto offset = i%8;
				if (b.color[vtx] == 3) {
					blacks[p] |= ((unsigned char)0x80 >> offset);
				} else if (b.color[vtx] == 2) {
					whites[p] |= ((unsigned char)0x80 >> offset);
				}
			}
		}

		// Break in case of 2 consecutive pass.
		if (next_move==PASS && prev_move==PASS) 
			return {};

		prev_move = next_move;
	}

	if (!to_move_is_black) {
		ft[1+46*16] = 1;
	}

	// get score 
	LGR lgr;
	int win_cnt = 0;
	int rollout_cnt = 1000;
	for(int i=0;i<rollout_cnt;++i){
		Board b_cpy = b;
		int result = PlayoutLGR(b_cpy, lgr, 7.5);
		if(result != 0) ++win_cnt;
	}
	bool is_black_win = ((double)win_cnt/rollout_cnt >= 0.5);

	char result = to_move_is_black == is_black_win ? 1 : -1;

	ft.back() = (unsigned char)result;

	return ft;
}


/**
 *  Play with random moves until the end and returns the result.
 *  white wins: 0, black wins: 1 or -1.
 */
int PlayoutRandom(Board& b, double komi) {

	int next_move;
	int prev_move = VNULL;
	int pl = b.my;

	while (b.move_cnt <= 720) {
		next_move = b.SelectRandomMove();
		// 2���A���Ńp�X�̏ꍇ�A�I�ǁD
		// Break in case of 2 consecutive pass.
		if (next_move==PASS && prev_move==PASS) break;
		prev_move = next_move;
	}

	// Return the result.
	return Win(b, pl, komi);

}

