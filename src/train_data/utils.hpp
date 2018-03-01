#pragma once

#include <vector>
#include <unordered_set>
#include <string>
#include <exception>
#include <cstdarg>
#include <algorithm>

namespace napp {

using std::vector;
using std::unordered_set;
using std::string;

void enumerateDirectory(vector<string> &out, const string &directory, bool recursive, const unordered_set<string>& exts={}, int max_count=-1);
vector<string> enumerateDirectory(const string &directory, bool recursive, const unordered_set<string>& exts={}, int max_count=-1);

vector<string> split_str(const string& name, char delim, bool strip=true);
string join_str(vector<string>::const_iterator begin, vector<string>::const_iterator end, char delim);



inline std::string format_str(const char *fmt, ...) {
    static char buf[2048];

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
#ifdef _MSC_VER
#pragma warning(default:4996)
#endif
    return std::string(buf);
}



vector<int> random_sample(int cap_size, int sample_size);

template<typename T>
inline vector<T> random_sample(const vector<T>& vt, int count) {
    auto indexes = random_sample(vt.size(), count);
    vector<T> out;
    for (auto i : indexes) 
        out.push_back(vt[i]);
    return out;
}
 

template<typename ITER>
inline int max_index(ITER begin, ITER end) {
    return std::distance(begin, std::max_element(begin, end));
}

template<typename T>
inline int max_index(const T& vt) {
    return max_index(vt.begin(), vt.end());
}



}
