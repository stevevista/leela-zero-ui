#include "zobrist.h"
#include "board.h"

std::random_device rd;
std::mt19937 mt_32(rd());
std::mt19937_64 mt_64(rd());

/**
 * [0.0, 1.0)�̈��l����������
 * double rand_d = mt_double(mt_32); �̂悤�Ɏg��.
 *
 * uniform real distribution of [0.0, 1.0).
 * Ex. double rand_d = mt_double(mt_32);
 */
std::uniform_real_distribution<double> mt_double(0.0, 1.0);

