#include <algorithm>
#include <assert.h>
#include <unordered_set>

#include "board.h"

/**
 *  ���͂̍��W�ɓ��ꏈ���������}�N��
 *  Macro that executes the same processing on surrounding vertexes.
 */
#define forEach4Nbr(v_origin,v_nbr,block) \
	int v_nbr;										\
	v_nbr = v_origin + 1;			block;			\
	v_nbr = v_origin - 1;			block;			\
	v_nbr = v_origin + EBSIZE;		block;			\
	v_nbr = v_origin - EBSIZE;		block;

#define forEach4Diag(v_origin,v_diag,block) \
	int v_diag;									\
	v_diag = v_origin + EBSIZE + 1;	block;		\
	v_diag = v_origin + EBSIZE - 1;	block;		\
	v_diag = v_origin - EBSIZE + 1;	block;		\
	v_diag = v_origin - EBSIZE - 1;	block;		

#define forEach8Nbr(v_origin,v_nbr,d_nbr,d_opp,block) \
	int v_nbr;	int d_nbr; int d_opp;											\
	v_nbr = v_origin + EBSIZE;			d_nbr = 0; d_opp = 2;		block;		\
	v_nbr = v_origin + 1;				d_nbr = 1; d_opp = 3;		block;		\
	v_nbr = v_origin - EBSIZE;			d_nbr = 2; d_opp = 0;		block;		\
	v_nbr = v_origin - 1;				d_nbr = 3; d_opp = 1;		block;		\
	v_nbr = v_origin + EBSIZE - 1;		d_nbr = 4; d_opp = 6;		block;		\
	v_nbr = v_origin + EBSIZE + 1;		d_nbr = 5; d_opp = 7;		block;		\
	v_nbr = v_origin - EBSIZE + 1;		d_nbr = 6; d_opp = 4;		block;		\
	v_nbr = v_origin - EBSIZE - 1;		d_nbr = 7; d_opp = 5;		block;

#define forEach12Nbr(v_origin,v_nbr,block)	 \
	int v_nbr;									\
	v_nbr = v_origin + EBSIZE;	block;			\
	v_nbr = v_origin + 1;		block;			\
	v_nbr = v_origin - EBSIZE;	block;			\
	v_nbr = v_origin - 1;		block;			\
	v_nbr = v_origin + EBSIZE - 1;	block;		\
	v_nbr = v_origin + EBSIZE + 1;	block;		\
	v_nbr = v_origin - EBSIZE + 1;	block;		\
	v_nbr = v_origin - EBSIZE - 1;	block;		\
	v_nbr = std::min(v_origin + 2 * EBSIZE, EBVCNT - 1);	block;	\
	v_nbr = v_origin + 2;									block;	\
	v_nbr = std::max(v_origin - 2 * EBSIZE, 0);				block;	\
	v_nbr = v_origin - 2;									block;


/**
 *  N�����z���̏������B��2�����̌^�̃T�C�Y���Ƃɏ��������Ă����B
 *  Initialize N-d array.
 */
template<typename A, size_t N, typename T>
inline void FillArray(A (&array)[N], const T &val){

    std::fill( (T*)array, (T*)(array+N), val );

}

#ifdef USE_SEMEAI
const double response_w[4][2] = {	{14.639, 1/14.639},
									{57.0406, 1/57.0406},
									{18.1951, 1/18.1951},
									{2.9047, 1/2.9047}};
#else
const double response_w[4][2] = {	{140.648, 1/140.648},
									{1241.99, 1/1241.99},
									{21.7774, 1/21.7774},
									{7.09814, 1/7.09814}};
#endif //USE_SEMEAI

const double semeai_w[2][2] = {{5.19799, 1/5.19799},
								{3.86838, 1/3.86838} };

Board::Board() {

	Clear();

}

Board::Board(const Board& other) {

	*this = other;

}

Board& Board::operator=(const Board& other) {

	my = other.my;
	her = other.her;
	std::memcpy(color, other.color, sizeof(color));
	std::memcpy(prev_color, other.prev_color, sizeof(prev_color));
	std::memcpy(empty, other.empty, sizeof(empty));
	std::memcpy(empty_idx, other.empty_idx, sizeof(empty_idx));
	std::memcpy(stone_cnt, other.stone_cnt, sizeof(stone_cnt));
	empty_cnt = other.empty_cnt;
	ko = other.ko;

	std::memcpy(ren, other.ren, sizeof(ren));
	std::memcpy(next_ren_v, other.next_ren_v, sizeof(next_ren_v));
	std::memcpy(ren_idx, other.ren_idx, sizeof(ren_idx));
	move_cnt = other.move_cnt;
	move_history.clear();
	copy(other.move_history.begin(), other.move_history.end(), back_inserter(move_history));
	std::memcpy(prev_move, other.prev_move, sizeof(prev_move));
	prev_ko = other.prev_ko;
	removed_stones.clear();
	copy(other.removed_stones.begin(), other.removed_stones.end(), back_inserter(removed_stones));
	std::memcpy(prob, other.prob, sizeof(prob));
	std::memcpy(ptn, other.ptn, sizeof(ptn));
	prev_ptn[0].bf = other.prev_ptn[0].bf;
	prev_ptn[1].bf = other.prev_ptn[1].bf;
	prev_ptn_prob = other.prev_ptn_prob;
	std::memcpy(response_move, other.response_move, sizeof(response_move));
	for(int i=0;i<2;++i){
		semeai_move[i].clear();
		copy(other.semeai_move[i].begin(), other.semeai_move[i].end(), back_inserter(semeai_move[i]));
	}
	std::memcpy(is_ptn_updated, other.is_ptn_updated, sizeof(is_ptn_updated));
	updated_ptns.clear();
	copy(other.updated_ptns.begin(), other.updated_ptns.end(), back_inserter(updated_ptns));
	std::memcpy(sum_prob_rank, other.sum_prob_rank, sizeof(sum_prob_rank));
	std::memcpy(pass_cnt, other.pass_cnt, sizeof(pass_cnt));

	return *this;

}


/**
 *  ������
 *  Initialize the board.
 */
void Board::Clear() {

	// ����-> (my, her) = (1, 0), ���� -> (0, 1).
	// if black's turn, (my, her) = (1, 0) else (0, 1).
	my = 1;
	her = 0;
	empty_cnt = 0;
	FillArray(sum_prob_rank, 0.0);
	FillArray(is_ptn_updated, false);

	for (int i=0;i<EBVCNT;++i) {

		next_ren_v[i] = i;
		ren_idx[i] = i;

		ren[i].SetNull();

		int ex = etox[i];
		int ey = etoy[i];

		// outer boundary
		if (ex == 0 || ex == EBSIZE - 1 || ey == 0 || ey == EBSIZE - 1) {
			color[i] = 1;
			for(auto& pc : prev_color) pc[i] = 1;
			empty_idx[i] = VNULL;	//442
			ptn[i].SetNull();		//0xffffffff

			prob[0][i] = prob[1][i] = 0;
		}
		// real board
		else {
			// empty vertex
			color[i] = 0;
			for(auto& pc : prev_color) pc[i] = 0;
			empty_idx[i] = empty_cnt;
			empty[empty_cnt] = i;
			++empty_cnt;
		}

	}

	for(int i=0;i<BVCNT;++i){

		int v = rtoe[i];

		ptn[v].Clear();	// 0x00000000

		// Set colors around v.
		forEach8Nbr(v, v_nbr8, d_nbr, d_opp, {
			ptn[v].SetColor(d_nbr, color[v_nbr8]);
		});

		// Initialize probability distribution.

		// Probability. Selected probability = prob[pl][v]/sum(prob[pl])
		prob[0][v] = prob_dist_base[v] * ptn[v].GetProb3x3(0,false);
		prob[1][v] = prob_dist_base[v] * ptn[v].GetProb3x3(1,false);

		// ���Ֆʂ�n�i�ڂ�prob�̘a
		// Sum of prob[pl][v] in the Nth rank on the real board.
		sum_prob_rank[0][rtoy[i]] += prob[0][v];
		sum_prob_rank[1][rtoy[i]] += prob[1][v];

	}

	stone_cnt[0] = stone_cnt[1] = 0;
	empty_cnt = BVCNT;
	ko = VNULL;
	move_cnt = 0;
	move_history.clear();
	removed_stones.clear();
	prev_move[0] = prev_move[1] = VNULL;
	prev_ko = VNULL;
	updated_ptns.clear();
	prev_ptn[0].SetNull();
	prev_ptn[1].SetNull();
	prev_ptn_prob = 0.0;
	for(auto& i: response_move) i = VNULL;
	for(auto& i: semeai_move) i.clear();
	pass_cnt[0] = pass_cnt[1] = 0;

}


/**
 *  ���Wv�ւ̒��肪���@�肩
 *  Return whether pl's move on v is legal.
 */
bool Board::IsLegal(int pl, int v) const{

	assert(v <= PASS);

	if (v == PASS) return true;
	if (color[v] != 0 || v == ko) return false;

	return ptn[v].IsLegal(pl);

}


/**
 *  ���Wv��pl���̊��`��
 *  Return whether v is an eye shape for pl.
 */
bool Board::IsEyeShape(int pl, int v) const{

	assert(color[v] == 0);

	if(ptn[v].IsEnclosed(pl)){

		// �΂ߗאڂ��鐔 {���_,�ՊO,����,����}
		// Counter of {empty, outer boundary, white, black} in diagonal positions.
		int diag_cnt[4] = {0, 0, 0, 0};

		for (int i=4;i<8;++i) {
			//4=NW, 5=NE, 6=SE, 7=SW
			++diag_cnt[ptn[v].ColorAt(i)];
		}

		// �΂߈ʒu�̓G�ΐ� + �ՊO�̌�
		// 2�ȏ��Ō����ڂɂȂ�
		// False eye if opponent's stones + outer boundary >= 2.
		int wedge_cnt = diag_cnt[int(pl==0) + 2] + int(diag_cnt[1] > 0);

		// �΂߈ʒu�̓G�΂������������Ƃ��A�����ڂ��珜�O
		// Return true if an opponent's stone can be taken immediately.
		if(wedge_cnt == 2){
			forEach4Diag(v, v_diag, {
				if(color[v_diag] == (int(pl==0) + 2)){
					if(	ren[ren_idx[v_diag]].IsAtari() &&
						ren[ren_idx[v_diag]].lib_atr != ko)
					{
						return true;
					}
				}
			});
		}
		// �����ڂłȂ��Ƃ��A���`�Ƃ݂Ȃ�
		// Return true if it is not false eye.
		else return wedge_cnt < 2;
	}

	return false;
}


/**
 *  ���Wv�������ڂ�
 *  Return whether v is a false eye.
 */
bool Board::IsFalseEye(int v) const{

	assert(color[v] == 0);

	// ���_���אڂ����Ƃ��͌����ڂłȂ�
	// Return false when empty vertexes adjoin.
	if(ptn[v].EmptyCnt() > 0) return false;

	// �אړ_�����ׂēG��or�Ւ[�łȂ��Ƃ��͌����ڂłȂ�
	// Return false when it is not enclosed by opponent's stones.
	if(!ptn[v].IsEnclosed(0) && !ptn[v].IsEnclosed(1)) return false;

	int pl = ptn[v].IsEnclosed(0)? 0 : 1;
	// Counter of {empty, outer boundary, white, black} in diagonal positions.
	int diag_cnt[4] = {0, 0, 0, 0};
	for (int i = 4; i < 8; ++i) {
		//4=NW, 5=NE, 6=SE, 7=SW
		++diag_cnt[ptn[v].ColorAt(i)];
	}

	// �΂߈ʒu�̓G�ΐ� + �ՊO�̌�
	// 2�ȏ��Ō����ڂɂȂ�
	// False eye if opponent's stones + outer boundary >= 2.
	int wedge_cnt = diag_cnt[int(pl==0) + 2] + int(diag_cnt[1] > 0);

	if(wedge_cnt == 2){
		forEach4Diag(v, v_diag, {
			if(color[v_diag] == (int(pl==0) + 2)){

				// �΂߈ʒu�̓G�΂������������Ƃ��Afalse
				// Not false eye if an opponent's stone can be taken immediately.
				if(ren[ren_idx[v_diag]].IsAtari()) return false;
			}
		});
	}

	return wedge_cnt >= 2;

}


/**
 *  ���Wv���Z�L���ǂ���
 *  Return whether v is Seki.
 */
bool Board::IsSeki(int v) const{

	assert(color[v] == 0);

	// �אڂ������_��2�ȏ� or �����̐΂��אڂ��Ȃ��Ƃ� -> false
	// Return false when empty vertexes are more than 2 or
	// both stones are not in neighboring positions.
	if(	!ptn[v].IsPreAtari()	||
		ptn[v].EmptyCnt() > 1 	||
		ptn[v].StoneCnt(0) == 0 ||
		ptn[v].StoneCnt(1) == 0	) return false;

	int64 lib_bits_tmp[6] = {0,0,0,0,0,0};
	std::vector<int> nbr_ren_idxs;
	for(int i=0;i<4;++i){
		int v_nbr = v + VSHIFT[i];	// neighboring position

		// when white or black stone
		if(color[v_nbr] > 1){

			// �ċz�_��2�łȂ� or �T�C�Y��1�̂Ƃ� -> false
			// Return false when the liberty number is not 2 or the size if 1.
			if(!ptn[v].IsPreAtari(i)) return false;
			else if(ren[ren_idx[v_nbr]].size == 1 &&
					ptn[v].StoneCnt(color[v_nbr] - 2) == 1){
				for(int j=0;j<4;++j){
					int v_nbr2 = v_nbr + VSHIFT[j];
					if(v_nbr2 != v && color[v_nbr2] == 0)
					{
						int nbr_cnt = ptn[v_nbr2].StoneCnt(color[v_nbr] - 2);
						if(nbr_cnt == 1) return false;
					}
				}
			}

			nbr_ren_idxs.push_back(ren_idx[v_nbr]);
		}
		else if(color[v_nbr] == 0){
			lib_bits_tmp[etor[v_nbr]/64] |= (0x1ULL << (etor[v_nbr] % 64));
		}
	}

	//int64 lib_bits_tmp[6] = {0,0,0,0,0,0};
	for(auto nbr_idx: nbr_ren_idxs)	{
		for(int i=0;i<6;++i){
			lib_bits_tmp[i] |= ren[nbr_idx].lib_bits[i];
		}
	}

	int lib_cnt = 0;
	for(auto lbt: lib_bits_tmp){
		if(lbt != 0){
			lib_cnt += (int)popcnt64(lbt);
		}
	}

	if(lib_cnt == 2){

		// �i�J�f�̃z�E���R�~���ł��邩
		// Check whether it is Self-Atari of Nakade.
		for(int i=0;i<6;++i){
			while(lib_bits_tmp[i] != 0){
				int ntz = NTZ(lib_bits_tmp[i]);
				int v_seki = rtoe[ntz + i * 64];
				if(IsSelfAtariNakade(v_seki)) return false;

				lib_bits_tmp[i] ^= (0x1ULL << ntz);
			}
		}
		return true;

	}
	else if(lib_cnt == 3){
		// �o���ɖڂ����������Z�L��
		// Check whether Seki where both Rens have an eye.
		int eye_cnt = 0;
		for(int i=0;i<6;++i){
			while(lib_bits_tmp[i] != 0){
				int ntz = NTZ(lib_bits_tmp[i]);
				int v_seki = rtoe[ntz + i * 64];
				if(IsEyeShape(0, v_seki) || IsEyeShape(1, v_seki)) ++eye_cnt;
				if(eye_cnt >= 2 || IsFalseEye(v_seki)) return true;

				lib_bits_tmp[i] ^= (0x1ULL << ntz);
			}
		}
	}

	return false;

}


/**
 *  ���Wv��update_ptn�ɒǉ�����
 *  Add v to the list of updated patterns.
 */
inline void Board::AddUpdatedPtn(int v) {

	if (!is_ptn_updated[v]) {
		//�m���X�V�t���O�𗧂Ă�
		is_ptn_updated[v] = true;

		//���W�ƌ��݂�ptn��update_ptns�ɓo�^
		updated_ptns.push_back(std::make_pair(v, ptn[v].bf));
	}

}


/**
 *  ���Wv���܂ޘA�̃A�^���n�_���X�V����
 *  Set Atari of the Ren including v.
 */
inline void Board::SetAtari(int v) {

	assert(color[v] > 1);

	int v_atari = ren[ren_idx[v]].lib_atr;

	AddUpdatedPtn(v_atari);
	ptn[v_atari].SetAtari(
		ren_idx[v_atari + EBSIZE] 	== 	ren_idx[v],
		ren_idx[v_atari + 1] 		== 	ren_idx[v],
		ren_idx[v_atari - EBSIZE]	== 	ren_idx[v],
		ren_idx[v_atari - 1] 		== 	ren_idx[v]
	);

}


/**
 *  ���Wv���܂ޘA��2�ċz�n�_���X�V����
 *  Set pre-Atari of the Ren including v.
 */
inline void Board::SetPreAtari(int v) {

	assert(color[v] > 1);

	int64 lib_bit = 0;
	for(int i=0;i<6;++i){
		lib_bit = ren[ren_idx[v]].lib_bits[i];
		while(lib_bit != 0){
			int ntz = NTZ(lib_bit);
			int v_patr = rtoe[ntz + i * 64];

			AddUpdatedPtn(v_patr);
			ptn[v_patr].SetPreAtari(
				ren_idx[v_patr + EBSIZE] 	== 	ren_idx[v],
				ren_idx[v_patr + 1] 		== 	ren_idx[v],
				ren_idx[v_patr - EBSIZE] 	== 	ren_idx[v],
				ren_idx[v_patr - 1] 		== 	ren_idx[v]
			);

			lib_bit ^= (0x1ULL << ntz);
		}
	}

}


/**
 *  ���Wv���܂ޘA�̃A�^���n�_����������
 *  Cancel Atari of the Ren including v.
 */
inline void Board::CancelAtari(int v) {

	assert(color[v] > 1);

	int v_atari = ren[ren_idx[v]].lib_atr;
	AddUpdatedPtn(v_atari);

	ptn[v_atari].CancelAtari(
		ren_idx[v_atari + EBSIZE] 	== 	ren_idx[v],
		ren_idx[v_atari + 1] 		== 	ren_idx[v],
		ren_idx[v_atari - EBSIZE] 	== 	ren_idx[v],
		ren_idx[v_atari - 1]		== 	ren_idx[v]
	);

}


/**
 *  ���Wv���܂ޘA��2�ċz�n�_����������
 *  Cancel pre-Atari of the Ren including v.
 */
inline void Board::CancelPreAtari(int v) {

	assert(color[v] > 1);

	int64 lib_bit = 0;
	for(int i=0;i<6;++i){
		lib_bit = ren[ren_idx[v]].lib_bits[i];
		while(lib_bit != 0){
			int ntz = NTZ(lib_bit);
			int v_patr = rtoe[ntz + i * 64];

			AddUpdatedPtn(v_patr);
			ptn[v_patr].CancelPreAtari(
				ren_idx[v_patr + EBSIZE] 	== 	ren_idx[v],
				ren_idx[v_patr + 1] 		== 	ren_idx[v],
				ren_idx[v_patr - EBSIZE] 	== 	ren_idx[v],
				ren_idx[v_patr - 1] 		== 	ren_idx[v]
			);

			lib_bit ^= (0x1ULL << ntz);
		}
	}

}


/**
 *  ���Wv�ɐ΂��u��
 *  Place a stone on position v.
 */
inline void Board::PlaceStone(int v) {

	assert(color[v] == 0);

	// 1. ����8�_��3x3�p�^�[�����X�V
	//    Update 3x3 patterns around v.
	color[v] = my + 2;
	forEach8Nbr(v, v_nbr8, d_nbr, d_opp, {
		if (color[v_nbr8] == 0) {
			is_ptn_updated[v_nbr8] = true;
			updated_ptns.push_back(std::make_pair(v_nbr8, ptn[v_nbr8].bf));
		}
		ptn[v_nbr8].SetColor(d_opp, color[v]);
	});

	// 2. �ΐ��A�m�����X�V
	//    Update stone number and probability at v.
	++stone_cnt[my];
	--empty_cnt;
	empty_idx[empty[empty_cnt]] = empty_idx[v];
	empty[empty_idx[v]] = empty[empty_cnt];
	ReplaceProb(my, v, 0.0);
	ReplaceProb(her, v, 0.0);

	// 3. v���܂ޘAindex�̍X�V
	//    Update Ren index including v.
	ren_idx[v] = v;
	ren[ren_idx[v]].Clear();

	// 4. ����4�_�̌ċz�_���X�V
	//    Update liberty on neighboring positions.
	forEach4Nbr(v, v_nbr, {
		// �ׂ����_�̂Ƃ��͎����̌ċz�_�𑝂₷
		// Add liberty when v_nbr is empty.
		if (color[v_nbr] == 0)	ren[ren_idx[v]].AddLib(v_nbr);
		// �����ȊO�ׂ̘͗A�̌ċz�_�����炷
		// Subtracts liberty in other cases.
		else  ren[ren_idx[v_nbr]].SubLib(v);
	});

}

/**
 *  ���Wv�̐΂�����
 *  Remove the stone on the position v.
 */
inline void Board::RemoveStone(int v) {

	assert(color[v] > 1);

	// 1. ����8�_��3x3�p�^�[�����X�V
	//    Update 3x3 patterns around v.
	color[v] = 0;
	is_ptn_updated[v] = true;
	ptn[v].ClearAtari();
	ptn[v].ClearPreAtari();
	forEach8Nbr(v, v_nbr8, d_nbr, d_opp, {
		if(color[v_nbr8] == 0) AddUpdatedPtn(v_nbr8);
		ptn[v_nbr8].SetColor(d_opp, 0);
	});

	// 2. �ΐ��A�m�����X�V
	//    Update stone number and probability at v.
	--stone_cnt[her];
	empty_idx[v] = empty_cnt;
	empty[empty_cnt] = v;
	++empty_cnt;
	ReplaceProb(my, v, prob_dist_base[v]);
	ReplaceProb(her, v, prob_dist_base[v]);
	ren_idx[v] = v;
	removed_stones.push_back(v);

}

/**
 *  ���Wv_base�Av_add���܂ޘA������
 *  v_add���܂ޘA��index������������
 *  Merge the Rens including v_base and v_add.
 *  Replace index of the Ren including v_add.
 */
inline void Board::MergeRen(int v_base, int v_add) {

	// 1. Ren�N���X�̌���
	//    Merge of Ren class.
	ren[ren_idx[v_base]].Merge(ren[ren_idx[v_add]]);

	// 2. ren_idx��v_base�ɏ���������
	//    Replace ren_idx of the Ren including v_add.
	int v_tmp = v_add;
	do {
		ren_idx[v_tmp] = ren_idx[v_base];
		v_tmp = next_ren_v[v_tmp];
	} while (v_tmp != v_add);

	// 3. next_ren_v����������
	//    Swap positions of next_ren_v.
	//
	//    (before)
	//    v_base: 0->1->2->3->0
	//    v_add : 4->5->6->4
	//    (after)
	//    v_base: 0->5->6->4->1->2->3->0
	std::swap(next_ren_v[v_base], next_ren_v[v_add]);

}

/**
 *  ���Wv���܂ޘA������
 *  Remove the Ren including v.
 */
inline void Board::RemoveRen(int v) {

	// 1. ���ׂĂ̐΂�����
	//    Remove all stones of the Ren.
	std::vector<int> spaces;	//�������΂̍��W���i�[����
	int v_tmp = v;
	do {
		RemoveStone(v_tmp);
		spaces.push_back(v_tmp);
		v_tmp = next_ren_v[v_tmp];
	} while (v_tmp != v);

	//    Check whether the space after removing stones is a Nakade shape.
	if(spaces.size()>=3 && spaces.size()<=6){
		unsigned long long space_hash = 0;
		for(auto v_space: spaces){

			//    Calculate Zobrist hash relative to the center position.
			space_hash ^= zobrist.hash[0][0][v_space - v + EBVCNT / 2];
		}
		if(nakade.vital.find(space_hash) != nakade.vital.end()){
			int v_nakade = v + nakade.vital.at(space_hash);
			if(v_nakade < EBVCNT){

				// b. �i�J�f�̋}�����o�^����
				//    Registers the vital of Nakade.
				response_move[0] = v_nakade;
			}
		}
	}

	//    Update liberty of neighboring Rens.
	std::vector<int> may_patr_list;		//�A�^�����������ꂽ�Aindex���i�[����
	do {
		forEach4Nbr(v_tmp, v_nbr, {
			if(color[v_nbr] >= 2){

				// �ċz�_���K���������̂ŃA�^���E2�ċz�_������
				// PlayLegal()�ł��̌��ɃA�^���E2�ċz�_���ēx�v�Z����
				// Cancel Atari or pre-Atari because liberty positions are added.
				// Final status of Atari or pre-Atari will be calculated in PlayLegal().
				if(ren[ren_idx[v_nbr]].IsAtari()){
					CancelAtari(v_nbr);
					may_patr_list.push_back(ren_idx[v_nbr]);
				}
				else if(ren[ren_idx[v_nbr]].IsPreAtari()) CancelPreAtari(v_nbr);

			}
			ren[ren_idx[v_nbr]].AddLib(v_tmp);
		});

		int v_next = next_ren_v[v_tmp];
		next_ren_v[v_tmp] = v_tmp;
		v_tmp = v_next;
	}while (v_tmp != v);

	//    Update liberty of Neighboring Rens.

	// Remove duplicated indexes.
	sort(may_patr_list.begin(), may_patr_list.end());
	may_patr_list.erase(unique(may_patr_list.begin(),may_patr_list.end()),may_patr_list.end());

	for(auto mpl_idx : may_patr_list){

		// Update ptn[] when liberty number is 2.
		if(ren[mpl_idx].IsPreAtari()){
			SetPreAtari(mpl_idx);
		}

	}

}

/**
 *  Return whether move on position v is self-Atari and forms Nakade.
 *  Use for checking whether Hourikomi is effective in Seki.
 */
inline bool Board::IsSelfAtariNakade(int v) const{

	// ����4�_�̘A���傫��<=4�̂Ƃ��A�i�J�f�ɂȂ邩�𒲂ׂ�
	// Check whether it will be Nakade shape when size of urrounding Ren is <= 4.
	std::vector<int> checked_idx[2];
	int64 space_hash[2] = {zobrist.hash[0][0][EBVCNT/2], zobrist.hash[0][0][EBVCNT/2]};
	bool under5[2] = {true, true};
	int64 lib_bits[2][6] = {{0,0,0,0,0,0}, {0,0,0,0,0,0}};

	forEach4Nbr(v,v_nbr,{
		if(color[v_nbr] >= 2){
			int pl = color[v_nbr] - 2;
			if(ren[ren_idx[v_nbr]].size < 5){
				if(find(checked_idx[pl].begin(), checked_idx[pl].end(), ren_idx[v_nbr])
					== checked_idx[pl].end())
				{
					checked_idx[pl].push_back(ren_idx[v_nbr]);

					for(int i=0;i<6;++i)
						lib_bits[pl][i] |= ren[ren_idx[v_nbr]].lib_bits[i];

					int v_tmp = v_nbr;
					do{
						// �Ւ��������̑��΍��W��zobrist�n�b�V��
						// Calculate Zobrist Hash relative to the center position.
						space_hash[pl] ^= zobrist.hash[0][0][v_tmp - v + EBVCNT/2];
						forEach4Nbr(v_tmp, v_nbr2, {
							if(	color[v_nbr2] == int(pl == 0) + 2){
								if(ren[ren_idx[v_nbr2]].lib_cnt != 2){
									under5[pl] = false;
									break;
								}
								else{
									for(int i=0;i<6;++i)
										lib_bits[pl][i] |= ren[ren_idx[v_nbr2]].lib_bits[i];
								}
							}
						});

						v_tmp = next_ren_v[v_tmp];
					} while(v_tmp != v_nbr);
				}
			}
			else under5[pl] = false;
		}
	});

	if(under5[0] && nakade.vital.find(space_hash[0]) != nakade.vital.end())
	{
		int lib_cnt = 0;
		for(auto lb:lib_bits[0]) lib_cnt += (int)popcnt64(lb);
		if(lib_cnt == 2) return true;
	}
	else if(under5[1] && nakade.vital.find(space_hash[1]) != nakade.vital.end())
	{
		int lib_cnt = 0;
		for(auto lb:lib_bits[1]) lib_cnt += (int)popcnt64(lb);
		if(lib_cnt == 2) return true;
	}

	return false;

}


inline bool Board::IsSelfAtari(int pl, int v) const{

	if(ptn[v].EmptyCnt() >= 2) return false;

	int64 lib_bits[6] = {0,0,0,0,0,0};
	int lib_cnt = 0;

	// v�ɗאڂ����A�̌ċz�_�𒲂ׂ�
	// Count number of liberty of the neighboring Rens.
	forEach4Nbr(v, v_nbr, {

		// ���_. Empty vertex.
		if(color[v_nbr] == 0){
			lib_bits[etor[v_nbr]/64] |= (0x1ULL<<(etor[v_nbr]%64));
		}
		// v�̗אڌ��_���G��. Opponent's stone.
		else if (color[v_nbr] == int(pl == 0)) {
			if (ren[ren_idx[v_nbr]].IsAtari()){
				if(ren[ren_idx[v_nbr]].size > 1) return false;
				lib_bits[etor[v_nbr]/64] |= (0x1ULL<<(etor[v_nbr]%64));
			}
		}
		// v�̗אڌ��_������. Player's stone.
		else if (color[v_nbr] == pl) {
			if(ren[ren_idx[v_nbr]].lib_cnt > 2) return false;
			for(int k=0;k<6;++k){
				lib_bits[k] |= ren[ren_idx[v_nbr]].lib_bits[k];
			}
		}

	});

	lib_bits[etor[v]/64] &= ~(0x1ULL<<(etor[v]%64));	// v�����O. Exclude v.
	for(int k=0;k<6;++k){
		if(lib_bits[k] != 0){
			lib_cnt += (int)popcnt64(lib_bits[k]);
		}
	}

	return (lib_cnt <= 1);
}


/**
 *  �ċz�_>2�̐΂̎��͂ɍU�ߍ����ł����΂��Ȃ������ׂ��D
 */
inline bool Board::Semeai2(std::vector<int>& patr_rens, std::vector<int>& her_libs){

	her_libs.clear();
	if(patr_rens.size() == 0) return false;
	int my_color = my + 2;

	// 1. �d���v�f���폜
	//    Remove duplicated indexes.
	sort(patr_rens.begin(), patr_rens.end());
	patr_rens.erase(unique(patr_rens.begin(),patr_rens.end()),patr_rens.end());

	for(auto patr_idx: patr_rens){

		if(ren[ren_idx[patr_idx]].size < 2) continue;
		int v_tmp = patr_idx;
		std::vector<int> tmp_her_libs;

		// 2. 2�ċz�_�ɂ��ꂽ�A�ɗאڂ����G�΂������邩���ׂ�
		//    Check whether it is possible to reduce liberty of neighboring stones of the Ren in pre-Atari.
		do {
			forEach4Nbr(v_tmp, v_nbr3,{

				Ren* ren_nbr = &ren[ren_idx[v_nbr3]];

				if(color[v_nbr3] == my_color){

					if(ren_nbr->IsAtari()){
						tmp_her_libs.clear();
						break;
					}
					else if(ren_nbr->IsPreAtari()){
						int64 lib_bit = 0;
						for(int i=0;i<6;++i){
							lib_bit = ren_nbr->lib_bits[i];
							while(lib_bit != 0){
								int ntz = NTZ(lib_bit);
								int v_patr = rtoe[ntz + i * 64];
								if(!IsSelfAtari(her, v_patr)){
									tmp_her_libs.push_back(v_patr);
								}

								lib_bit ^= (0x1ULL << ntz);
							}
						}
					}

				}

			});
			v_tmp = next_ren_v[v_tmp];
		} while (v_tmp != patr_idx);

		if(tmp_her_libs.size() > 0){
			for(auto thl:tmp_her_libs) her_libs.push_back(thl);
		}
	}

	return (her_libs.size() > 0);

}


/**
 *  �ċz�_3�̐΂̎��͂ɍU�ߍ����ł����΂��Ȃ������ׂ��D
 */
inline bool Board::Semeai3(std::vector<int>& lib3_rens, std::vector<int>& her_libs){

	her_libs.clear();
	if(lib3_rens.size() == 0) return false;
	int my_color = my + 2;

	// 1. �d���v�f���폜
	//    Remove duplicated indexes.
	sort(lib3_rens.begin(), lib3_rens.end());
	lib3_rens.erase(unique(lib3_rens.begin(),lib3_rens.end()),lib3_rens.end());

	for(auto patr_idx: lib3_rens){

		if(ren[ren_idx[patr_idx]].size < 3) continue;
		int v_tmp = patr_idx;
		std::vector<int> tmp_her_libs;

		// 2. 3�ċz�_�ɂ��ꂽ�A�ɗאڂ����G�΂������邩���ׂ�
		//    Check whether it is possible to reduce liberty of neighboring stones of the Ren with 3 liberties.
		do {
			forEach4Nbr(v_tmp, v_nbr3,{

				Ren* ren_nbr = &ren[ren_idx[v_nbr3]];

				if(color[v_nbr3] == my_color){

					if(ren_nbr->IsAtari()){
						tmp_her_libs.clear();
						break;
					}
					else if(ren_nbr->lib_cnt <= 3){
						int64 lib_bit = 0;
						for(int i=0;i<6;++i){
							lib_bit = ren_nbr->lib_bits[i];
							while(lib_bit != 0){
								int ntz = NTZ(lib_bit);
								int v_patr = rtoe[ntz + i * 64];
								if(!IsSelfAtari(her, v_patr)){
									tmp_her_libs.push_back(v_patr);
								}

								lib_bit ^= (0x1ULL << ntz);
							}
						}
					}

				}

			});
			v_tmp = next_ren_v[v_tmp];
		} while (v_tmp != patr_idx);

		if(tmp_her_libs.size() > 0){
			for(auto thl:tmp_her_libs) her_libs.push_back(thl);
		}
	}

	return (her_libs.size() > 0);

}


/**
 *  ���Wv�ɒ��肷��
 *  ���肪���@�肩�͎��O�ɕ]�����Ă����K�v������.
 *  Update the board with the move on position v.
 *  It is necessary to confirm in advance whether the move is legal.
 */
void Board::PlayLegal(int v) {

	assert(v <= PASS);
	assert(v == PASS || color[v] == 0);
	assert(v != ko);

	// 1. �����������X�V
	//    Update history.
	int prev_empty_cnt = empty_cnt;
	bool is_in_eye = ptn[v].IsEnclosed(her);
	prev_ko = ko;
	ko = VNULL;
	move_history.push_back(v);
	++move_cnt;
	removed_stones.clear();
#ifdef USE_52FEATURE
	for(int i=6;i>0;--i){
		std::memcpy(prev_color[i], prev_color[i - 1], sizeof(prev_color[i]));
	}
	std::memcpy(prev_color[0], color, sizeof(prev_color[0]));
#endif

	// 2. �O�̒����A12�_�p�^�[���̊m��������
	//    Restore probability of distance and 12-point pattern
	//    of the previous move.
	SubProbDist();
	SubPrevPtn();

	// 3. response_move�̊m��������
	//    Restore probability of response_move.
	response_move[0] = VNULL;	//�i�J�f�̋}��. Vital of Nakade.
	if(response_move[1] != VNULL){
		AddProb(my, response_move[1], response_w[1][1]);
		response_move[1] = VNULL;	//�A�^���̎��͂̐΂�����. Save Atari stones by taking opponent's stones.
	}
	if(response_move[2] != VNULL){
		AddProb(my, response_move[2], response_w[2][1]);
		response_move[2] = VNULL;	//�A�^���𓦂���. Save Atari stones by escaping move.
	}
	if(response_move[3] != VNULL){
		AddProb(my, response_move[3], response_w[3][1]);
		response_move[3] = VNULL;	//���O�̐΂�����. Take a stone placed previously.
	}
#ifdef USE_SEMEAI
	if(semeai_move[0].size() > 0){
		for(auto sm: semeai_move[0]) AddProb(my, sm, semeai_w[0][1]);
		semeai_move[0].clear();
	}
	if(semeai_move[1].size() > 0){
		for(auto sm: semeai_move[1]) AddProb(my, sm, semeai_w[1][1]);
		semeai_move[1].clear();
	}
#endif

	if (v == PASS) {
		++pass_cnt[my];
		prev_move[my] = v;
		prev_ptn[1] = prev_ptn[0];
		prev_ptn[0].SetNull();

		// ���Ԃ������ւ���
		// Exchange the turn indexes.
		my = int(my == 0);
		her = int(her == 0);

		return;
	}

	// 4. �m���X�V�t���O��������
	//    Initialize the updating flag of 3x3 pattern.
	FillArray(is_ptn_updated, false);
	updated_ptns.clear();

	// 5. response�p�^�[�����X�V���A�΂��u��
	//    Update response pattern and place a stone.
	UpdatePrevPtn(v);
	PlaceStone(v);

	// 6. ���΂ƌ���
	//    Merge the stone with other Rens.
	int my_color = my + 2;
	forEach4Nbr(v, v_nbr1, {

		// a. ���΂��قȂ�ren_idx�̂Ƃ�
		//    When v_nbr1 is my stone color and another Ren.
		if(color[v_nbr1] == my_color && ren_idx[v_nbr1] != ren_idx[v]){

			// b. �A�^���ɂȂ����Ƃ��A�v���A�^��������
			//    Cancel pre-Atari when it becomes in Atari.
			if(ren[ren_idx[v_nbr1]].lib_cnt == 1) CancelPreAtari(v_nbr1);

			// c. �ΐ������������x�[�X�Ɍ���
			//    Merge them with the larger size of Ren as the base.
			if (ren[ren_idx[v]].size > ren[ren_idx[v_nbr1]].size) {
				MergeRen(v, v_nbr1);
			}
			else MergeRen(v_nbr1, v);

		}

	});

	// 7. �G�΂̌ċz�_�����炷
	//    Reduce liberty of opponent's stones.
	int her_color = int(my == 0) + 2;
	std::vector<int> atari_rens;
	std::vector<int> patr_rens;
	std::vector<int> lib3_rens;

	forEach4Nbr(v, v_nbr2, {

		// �G�΂̂Ƃ�. If an opponent stone.
		if (color[v_nbr2] == her_color) {
			switch(ren[ren_idx[v_nbr2]].lib_cnt)
			{
			case 0:
				RemoveRen(v_nbr2);
				break;
			case 1:
				SetAtari(v_nbr2);
				atari_rens.push_back(ren_idx[v_nbr2]);
				break;
			case 2:
				SetPreAtari(v_nbr2);
				patr_rens.push_back(ren_idx[v_nbr2]);
				break;
			case 3:
				lib3_rens.push_back(ren_idx[v_nbr2]);
				break;
			default:
				break;
			}
		}

	});

	// 8. �R�E���X�V
	//    Update Ko.
	if (is_in_eye && prev_empty_cnt == empty_cnt) {
		ko = empty[empty_cnt - 1];
	}

	// 9. �����A�̃A�^��or�Q�ċz�_�������X�V
	//    Update Atari/pre-Atari of the Ren including v.
	switch(ren[ren_idx[v]].lib_cnt){
	case 1:
		SetAtari(v);
		if(ren[ren_idx[v]].lib_atr != ko){
			response_move[3] = ren[ren_idx[v]].lib_atr;
		}
		break;
	case 2:
		SetPreAtari(v);
		break;
	default:
		break;
	}

	// 10. �A�^���ɂȂ����΂������������X�V
	//     Update response_move which saves stones in Atari.
	if(atari_rens.size() > 0){

		// a. �d���v�f���폜
		//    Remove duplicated indexes.
		sort(atari_rens.begin(), atari_rens.end());
		atari_rens.erase(unique(atari_rens.begin(),atari_rens.end()),atari_rens.end());

		for(auto atr_idx: atari_rens){

			int v_atari = ren[atr_idx].lib_atr;
			int v_tmp = atr_idx;
			int max_stone_cnt = 0;

			// b. �A�^���ɂ��ꂽ�A�ɗאڂ����G�΂������邩���ׂ�
			//    Check whether it is possible to take neighboring stones of the Ren in Atari.
			do {
				forEach4Nbr(v_tmp, v_nbr3,{

					Ren* ren_nbr = &ren[ren_idx[v_nbr3]];

					if(color[v_nbr3] == my_color &&
						ren_nbr->IsAtari() &&
						ren_nbr->lib_atr != ko)
					{
						bool inner_cap = false;

						if(ren_nbr->size == 1){
							int cap_cnt = 0;
							int v_s = ren_nbr->lib_atr;
							for(int i=0;i<4;++i){
								if(ptn[v_s].IsAtari(i) && ptn[v_s].ColorAt(i) == my_color) ++cap_cnt;
							}
							if(cap_cnt <= 1 && v_s == ren[atr_idx].lib_atr) inner_cap = true;
						}

						if(!inner_cap && ren_nbr->size > max_stone_cnt){
							response_move[1] = ren_nbr->lib_atr;
							max_stone_cnt = ren_nbr->size;
						}
					}

				});
				v_tmp = next_ren_v[v_tmp];
			} while (v_tmp != atr_idx);

			// c. �A�^�����瓦�����邩���ׂ�
			//    Check whether it is possible to save stones by escaping.
			if(ptn[v_atari].EmptyCnt() == 2){
				if(ladder_ptn[her].find(ptn[v_atari].bf) == ladder_ptn[her].end()){
					response_move[2] = v_atari;
				}
			}
			else if(ptn[v_atari].EmptyCnt() > 2 || !IsSelfAtari(her, v_atari)){
				response_move[2] = v_atari;
			}
		}
	}

	// 10-1. �U�ߍ����̓_�𒲂ׂ�.
#ifdef USE_SEMEAI
	if(patr_rens.size() > 0) Semeai2(patr_rens, semeai_move[0]);
	if(lib3_rens.size() > 0) Semeai3(lib3_rens, semeai_move[1]);
#endif

	// 11. �p�^�[���ɕύX���������n�_�̊m�����X�V
	//     Update probability on all vertexes where 3x3 patterns were changed.
	UpdateProbAll();

	// 12. ���X�|���X�p�^�[���̊m�����X�V
	//     Update probability of the response pattern.
	prev_ptn_prob = 0.0;
	if(prob_ptn_rsp.find(prev_ptn[0].bf) != prob_ptn_rsp.end()){
		auto add_prob = prob_ptn_rsp.at(prev_ptn[0].bf);
		prev_ptn_prob = add_prob[1];	//���X�|���X���Ԃ��猳�ɖ߂��m���p�����[�^
		forEach12Nbr(v, v_nbr12, {
			AddProb(her, v_nbr12, add_prob[0]);
		});
	}

	// 13. ���̑��̊m�����X�V
	//     Update probability based on distance and response moves.
	AddProbDist(v);

	if(response_move[1] != VNULL){
		AddProb(her, response_move[1], response_w[1][0]);
	}
	if(response_move[2] != VNULL){
		AddProb(her, response_move[2], response_w[2][0]);
	}
	if(response_move[3] != VNULL){
		AddProb(her, response_move[3], response_w[3][0]);
	}
#ifdef USE_SEMEAI
	if(semeai_move[0].size() > 0){
		for(auto sm: semeai_move[0]) AddProb(her, sm, semeai_w[0][0]);
	}
	if(semeai_move[1].size() > 0){
		for(auto sm: semeai_move[1]) AddProb(her, sm, semeai_w[1][0]);
	}
#endif

	// 14. ���ԕύX
	//     Exchange the turn indexes.
	prev_move[my] = v;
	my = int(my == 0);
	her = int(her == 0);

}


/**
 *  v���ӂ�12�_�p�^�[����prev_ptn�ɓo�^����
 *  Register 12-point pattern around v.
 */
inline void Board::UpdatePrevPtn(int v){

	// 1. prev_ptn[1]��12�_�p�^�[���������R�s�[
	//    Copy the pattern to prev_ptn[1].
	prev_ptn[1] = prev_ptn[0];

	// 2. 3x3�p�^�[������(2,0)�ʒu�̐Ώ������ǉ�
	//    Add stones in position of the Manhattan distance = 4.
	prev_ptn[0] = ptn[v];
	prev_ptn[0].SetColor(8, color[std::min(v + EBSIZE * 2, EBVCNT - 1)]);
	prev_ptn[0].SetColor(9, color[v + 2]);
	prev_ptn[0].SetColor(10, color[std::max(v - EBSIZE * 2, 0)]);
	prev_ptn[0].SetColor(11, color[v - 2]);

	// 3. ���Ԃ̂Ƃ��΂̐F�𔽓]������
	//    Flip color if it is black's turn.
	if(my == 1) prev_ptn[0].FlipColor();

}

/**
 *  ���X�|���X�p�^�[���ɂ����m���w�W����������
 *  Restore probability of response pattern.
 */
inline void Board::SubPrevPtn(){

	if(prev_ptn_prob == 0.0 || prev_move[her] >= PASS) return;

	// ����12�_�̊m�������ɖ߂�
	// Restore the probability of 12 surroundings.
	forEach12Nbr(prev_move[her], v_nbr, {
		AddProb(my, v_nbr, prev_ptn_prob);
	});

}

/**
 *  ���Wv��pl���̎��m����new_prob�Œu��������
 *  Replace the probability on position v with new_prob.
 */
void Board::ReplaceProb(int pl, int v, double new_prob) {

	sum_prob_rank[pl][etoy[v] - 1] += new_prob - prob[pl][v];
	prob[pl][v] = new_prob;

}

/**
 *  ���Wv��pl���̊m���p�����[�^��add_prob�����Z����
 *  Multiply probability.
 */
inline void Board::AddProb(int pl, int v, double add_prob) {

	if(v >= PASS || prob[pl][v] == 0) return;

	sum_prob_rank[pl][etoy[v] - 1] += (add_prob - 1) * prob[pl][v];
	prob[pl][v] *= add_prob;

}


/**
 *  �ՖʑS�̂�12�_�p�^�[���Ƌ����p�����[�^���X�V����
 *  Update probability based on parameters of 12-point patterns and long distance
 *  on the all vertexes.
 */
void Board::AddProbPtn12() {

	for(int i=0;i<empty_cnt;++i){
		int v = empty[i];

		int prev_move_ = (move_cnt >= 3)? move_history[move_cnt - 3] : PASS;

		// 1. �����p�����[�^���ǉ�
		//    Add probability of long distance.
		AddProb(my, v, prob_dist[0][DistBetween(v, prev_move[her])][0]);
		AddProb(my, v, prob_dist[1][DistBetween(v, prev_move[my])][0]);
		AddProb(her, v, prob_dist[0][DistBetween(v, prev_move[my])][0]);
		AddProb(her, v, prob_dist[1][DistBetween(v, prev_move_)][0]);

		// 2. 12�_�p�^�[���̊m�����ǉ�
		//    Add probability of 12-point patterns.
		Pattern3x3 tmp_ptn = ptn[v];
		tmp_ptn.SetColor(8, color[std::min(EBVCNT - 1, (v + EBSIZE * 2))]);
		tmp_ptn.SetColor(9, color[v + 2]);
		tmp_ptn.SetColor(10, color[std::max(0, (v - EBSIZE * 2))]);
		tmp_ptn.SetColor(11, color[v - 2]);
		if(prob_ptn12.find(tmp_ptn.bf) != prob_ptn12.end()){
			AddProb(my, v, prob_ptn12[tmp_ptn.bf][my]);
			AddProb(her, v, prob_ptn12[tmp_ptn.bf][her]);
		}
	}

}

/**
 *  �ՖʑS��3x3�Ńp�^�[�����Čv�Z����
 *  Recalculate 3x3-pattern probability on the all vertexes.
 */
void Board::RecalcProbAll(){

	for(int i=0;i<empty_cnt;++i){
		int v = empty[i];
		for(int j=0;j<2;++j){
			ReplaceProb(j, v, prob_dist_base[v] * ptn[v].GetProb3x3(j,false));
		}
	}

}

/**
 *  3x3�p�^�[���ɕύX���������_�̊m�����X�V����
 *  Update probability on all vertexes where 3x3 patterns were changed.
 */
inline void Board::UpdateProbAll() {

	// 1. 3x3�p�^�[���ɕύX�����������_�̊m�����X�V
	//    Update probability on empty vertexes where 3x3 pattern was changed.
	for (auto i: updated_ptns) {
		for (int j=0;j<2;++j) {
			int v = i.first;
			int bf = i.second;
			Pattern3x3 ptn_(bf);
			double ptn_prob = ptn[v].GetProb3x3(j,false);
			double prob_diff = ptn_.GetProb3x3(j,true) * ptn_prob;

			if(prob[j][v] == 0){
				ReplaceProb(j, v, prob_dist_base[v] * ptn_prob);
			}
			else{
				AddProb(j, v, prob_diff);
			}
		}
	}

	//2. �΂������ꂽ�n�_�̊m�����X�V
	//   prob_dist_base��RemoveStone�Œǉ���
	//   Update probability on positions where a stone was removed.
	//   Probability of prob_dist_base has been already added in RemoveStone().
	for (auto i: removed_stones) {
		AddProb(my, i, ptn[i].GetProb3x3(my,false));
		AddProb(her, i, ptn[i].GetProb3x3(her,false));
	}

}

/**
 *  Subtract probability weight based on short distance.
 *  Neighbor 8 points.
 */
inline void Board::SubProbDist() {

	if (prev_move[her] < PASS) {
		forEach8Nbr(prev_move[her],v_nbr,d_nbr,d_opp,{
			AddProb(my, v_nbr, response_w[0][1]);
		});
	}

}

/**
 *  Add probability weight based on short distance.
 *  Neighbor 8 points.
 */
inline void Board::AddProbDist(int v) {

	forEach8Nbr(v,v_nbr,d_nbr,d_opp,{
		AddProb(her, v_nbr, response_w[0][0]);
	});

}

/**
 *  Select and play a legal move randomly.
 *  Return the selected move.
 */
int Board::SelectRandomMove() {

	int i0 = int(mt_double(mt_32) * empty_cnt);	 // [0, empty_cnt)
	int i = i0;
	int next_move = PASS;

	for (;;) {
		next_move = empty[i];

		// Break if next_move is legal and dosen't fill an eye and Seki.
		if ( !IsEyeShape(my, next_move) 	&&
			 IsLegal(my, next_move) 	&&
			 !IsSeki(next_move)			) break;

		++i;
		if(i == empty_cnt) i = 0;

		// Repeat until all empty vertexes are checked.
		if (i == i0) {
			next_move = PASS;
			break;
		}
	}

	PlayLegal(next_move);

	return next_move;

}

/**
 *  Select and play a legal move based on probability distribution.
 *  Return the selected move.
 */
int Board::SelectMove() {

	int next_move = PASS;
	double rand_double = mt_double(mt_32); // [0.0, 1.0)
	double prob_rank[BSIZE];
	double prob_v[EBVCNT];
	int i, x, y;
	double rand_move, tmp_sum;

	//    Copy probability.
	std::memcpy(prob_rank, sum_prob_rank[my], sizeof(prob_rank));
	std::memcpy(prob_v, prob[my], sizeof(prob_v));
	double total_sum = 0.0;
	for (auto rs: prob_rank) total_sum += rs;

	//    Select next move based on probability distribution.
	for (i=0;i<empty_cnt;++i) {
		if (total_sum <= 0) break;
		rand_move = rand_double * total_sum;
		tmp_sum = 0;

		//    Find the rank where sum of prob_rank exceeds rand_move.
		for (y=0;y<BSIZE;++y) {
			tmp_sum += prob_rank[y];
			if (tmp_sum > rand_move) break;
		}
		tmp_sum -= prob_rank[y];
		assert(y < BSIZE);

		//    Find the position where sum of probability exceeds rand_move.
		next_move = xytoe[1][y + 1];
		for (x=0;x<BSIZE;++x) {
			tmp_sum += prob_v[next_move];
			if (tmp_sum > rand_move){
				break;
			}
			++next_move;
		}
		//assert(x < BSIZE);

		//    Break if next_move is legal and dosen't fill an eye or Seki.
		if (IsLegal(my, next_move) 		&&
			!IsEyeShape(my, next_move) 	&&
			!IsSeki(next_move)			) break;

		//    Recalculate after subtracting probability of next_move.
		prob_rank[y] -= prob_v[next_move];
		total_sum -= prob_v[next_move];
		prob_v[next_move] = 0;
		next_move = PASS;
	}

	//    Play next_move.
	PlayLegal(next_move);

	return next_move;

}
