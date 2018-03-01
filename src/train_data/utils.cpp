

#include <iostream>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif
#include <fstream>
#include <cassert>
#include <cstring>
#include <cstdarg>
#include <sstream>
#include "utils.hpp"
#include <random>
#include <algorithm> 

namespace napp {


void enumerateDirectory(vector<string> &out, const string &directory, bool recursive, const unordered_set<string>& exts, int max_count)
{
#ifdef _WIN32
    HANDLE dir;
    WIN32_FIND_DATA file_data;

    if ((dir = FindFirstFile((directory + "/*").c_str(), &file_data)) == INVALID_HANDLE_VALUE)
        return; /* No files found */

    do {
        const string file_name = file_data.cFileName;
        const string full_file_name = directory + "/" + file_name;
        const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (max_count >0 && out.size() >= max_count)
            return;

        if (file_name[0] == '.')
            continue;

        if (is_directory && !recursive)
            continue;

        if (is_directory) 
            enumerateDirectory(out, full_file_name, recursive);
        else {
            if (!exts.empty()) {
                auto ext = file_name.substr(file_name.rfind(".")+1);
                if (exts.find(ext) == exts.end())
                    continue;
            }
            out.push_back(full_file_name);
        }

    } while (FindNextFile(dir, &file_data));

    FindClose(dir);
#else
    DIR *dir;
    class dirent *ent;
    class stat st;

    dir = opendir(directory.c_str());
    if (!dir)
        return;

    while ((ent = readdir(dir)) != NULL) {
        const std::string file_name = ent->d_name;
        const std::string full_file_name = directory + "/" + file_name;

        if (max_count >0 && out.size() >= max_count)
            return;

        if (file_name[0] == '.')
            continue;

        if (stat(full_file_name.c_str(), &st) == -1)
            continue;

        const bool is_directory = (st.st_mode & S_IFDIR) != 0;

        if (is_directory && !recursive)
            continue;

        if (is_directory) 
            enumerateDirectory(out, full_file_name, recursive);
        else {
            if (!exts.empty()) {
                auto ext = file_name.substr(file_name.rfind(".")+1);
                if (exts.find(ext) == exts.end())
                    continue;
            }
            out.push_back(full_file_name);
        }
    }
    closedir(dir);
#endif
} // GetFilesInDirectory


vector<string> enumerateDirectory(const string &directory, bool recursive, const unordered_set<string>& exts, int max_count) {
    vector<string> files;
    enumerateDirectory(files, directory, recursive, exts, max_count);
    return files;
}


vector<string> split_str(const string& name, char delim, bool strip) {

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


string join_str(vector<string>::const_iterator begin, vector<string>::const_iterator end, char delim) {
    std::string s;
    for (auto it=begin; it!=end; it++) {
        s += *it;
        s += delim;
    }
    if (!s.empty())
        s.erase(s.end()-1);
    return s;
}



vector<int> random_sample(int cap_size, int sample_size) {

    vector<int> indexes(cap_size);
    for (int i=0; i<cap_size; i++) indexes[i] = i;

    std::random_shuffle(indexes.begin(), indexes.end());
    return vector<int>(indexes.begin(), indexes.begin()+ std::min(cap_size, sample_size));
}


}

