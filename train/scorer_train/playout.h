#pragma once

#include <array>
#include <vector>
#include <unordered_map>

#include "board.h"


// Specialization of hash function.
namespace std {
	template<typename T>
	struct hash<array<T, 4>> {
		size_t operator()(const array<T, 4>& p) const {
			size_t seed = 0;
			hash<T> h;
			seed ^= h(p[0]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= h(p[1]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= h(p[2]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= h(p[3]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
}


struct LGR{

	// Array that has LGR moves obtained by rollout.
	std::array<std::array<std::array<int, EBVCNT>, EBVCNT>, 2> rollout;

	LGR(){ Clear(); }
	LGR(const LGR& other){ *this = other; }

	void Clear(){
		for(auto& r1:rollout) for(auto& r2:r1) for(auto& r3:r2){ r3 = VNULL; }
	}

	LGR& operator=(const LGR& rhs){
		for(int i=0;i<2;++i){
			for(int j=0;j<EBVCNT;++j){
				for(int k=0;k<EBVCNT;++k){
					rollout[i][j][k] = rhs.rollout[i][j][k];
				}
			}
		}
		return *this;
	}

	LGR& operator+=(const LGR& rhs){
		for(int i=0;i<2;++i){
			for(int j=0;j<EBVCNT;++j){
				for(int k=0;k<EBVCNT;++k){
					if(rhs.rollout[i][j][k] != VNULL)
						rollout[i][j][k] = rhs.rollout[i][j][k];
				}
			}
		}
		return *this;
	}
};


int PlayoutLGR(Board& b, LGR& lgr, double komi=KOMI);

std::vector<unsigned char> generate_sample();

extern bool japanese_rule;
