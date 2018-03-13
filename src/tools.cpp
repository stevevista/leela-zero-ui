#include "tools.h"
#include "lz/GTP.h"

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
#include <random>
#include <algorithm> 
#include <iterator>
#include <cmath>
#include <algorithm>
#include <array>
#include <cassert>
#include <memory>

using namespace std;



static vector<string> listFiles(const string &directory)
{
    vector<string> out;
#ifdef _WIN32
    HANDLE dir;
    WIN32_FIND_DATA file_data;

    if ((dir = FindFirstFile((directory + "/*").c_str(), &file_data)) == INVALID_HANDLE_VALUE)
        return {}; /* No files found */

    do {
        const string file_name = file_data.cFileName;
        const string full_file_name = directory + "/" + file_name;
        const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (!is_directory && file_name[0] != '.')
            out.push_back(full_file_name);

    } while (FindNextFile(dir, &file_data));

    FindClose(dir);
#else
    DIR *dir;
    class dirent *ent;
    class stat st;

    dir = opendir(directory.c_str());
    if (!dir)
        return {};

    while ((ent = readdir(dir)) != NULL) {
        const std::string file_name = ent->d_name;
        const std::string full_file_name = directory + "/" + file_name;

        if (stat(full_file_name.c_str(), &st) == -1)
            continue;

        const bool is_directory = (st.st_mode & S_IFDIR) != 0;

        if (!is_directory && file_name[0] != '.')
            out.push_back(full_file_name);
    }
    closedir(dir);
#endif
    return out;
} // GetFilesInDirectory


static 
std::pair<int, int>  parse_v1_network(std::ifstream& wtfile) {

    // First line was the version number
    auto linecount = size_t{1};
    auto channels = 0;
    auto line = std::string{};
    while (std::getline(wtfile, line)) {
        auto iss = std::stringstream{line};
        // Third line of parameters are the convolution layer biases,
        // so this tells us the amount of channels in the residual layers.
        // We are assuming all layers have the same amount of filters.
        if (linecount == 2) {
            auto count = std::distance(std::istream_iterator<std::string>(iss),
                                       std::istream_iterator<std::string>());
            channels = count;
        }
        linecount++;
    }
    // 1 format id, 1 input layer (4 x weights), 14 ending weights,
    // the rest are residuals, every residual has 8 x weight lines
    auto residual_blocks = linecount - (1 + 4 + 14);
    if (residual_blocks % 8 != 0) {
        return {0, 0};
    }
    residual_blocks /= 8;
    wtfile.close();

    return {channels, residual_blocks};
}

string findPossibleWeightsFile(const string &directory) {

    auto flist = listFiles(directory);

    size_t max_residual_blocks = 0;
    string select_file;

    for (auto fullpath : flist) {
        auto ext = fullpath.substr(fullpath.rfind(".")+1);
        if (ext == "txt") {
            auto format_version = -1;
            size_t channels = 0, residual_blocks;

            auto wtfile = std::ifstream{fullpath};
            if (wtfile) {
                auto line = std::string{};
                if (std::getline(wtfile, line)) {
                    if (line.size() < 4)
                        format_version = stoi(line);

                    if (format_version == 1) {
                
                        std::tie(channels, residual_blocks) = parse_v1_network(wtfile);
                        if (channels != 0) {
                            cerr << "Found weights: " << fullpath << endl;
                            cerr << "channels: " << channels << endl;
                            cerr << "residual_blocks: " << residual_blocks << endl;

                            if (channels > max_residual_blocks) {
                                max_residual_blocks = channels;
                                select_file = fullpath;
                            }
                        }
                    }
                }
                wtfile.close();
            }
        }
    }

    if (select_file.size())
        cerr << "Select weights: " << select_file << endl;
    return select_file;
}


void parseLeelaZeroArgs(int argc, char **argv, vector<string>& players) {

    string append_str;

    string selfpath = argv[0];
    auto pos  = selfpath.rfind(
        #ifdef _WIN32
        '\\'
        #else
        '/'
        #endif
        );

    selfpath = selfpath.substr(0, pos); 


    for (int i=1; i<argc; i++) {
        string opt = argv[i];

        if (opt == "...") {
            for (int j=i+1; j<argc; j++) {
                append_str += " ";
                append_str += argv[j];
            }
            continue;
        }
        
        if (opt == "--gtp" || opt == "-g") {
            cfg_gtp_mode = true;
        }
        else if (opt == "--player") {
            string player = argv[++i];
            if (player.find(" ") == string::npos && player.find(".txt") != string::npos) {
#ifdef _WIN32
                player = "leelaz.exe -g -w " + player;
#else
                player = "./leelaz -g -w " + player;
#endif
            }
            players.push_back(player);
        }
        else if (opt == "--threads" || opt == "-t") {
            int num_threads = std::stoi(argv[++i]);
            if (num_threads > cfg_num_threads) {
                fprintf(stderr, "Clamping threads to maximum = %d\n", cfg_num_threads);
            } else if (num_threads != cfg_num_threads) {
                fprintf(stderr, "Using %d thread(s).\n", num_threads);
                cfg_num_threads = num_threads;
            }
        }
        else if (opt == "--playouts" || opt == "-p") {
            cfg_max_playouts = std::stoi(argv[++i]);
        }
        else if (opt == "--noponder") {
            cfg_allow_pondering = false;
        }
        else if (opt == "--visits" || opt == "-v") {
            cfg_max_visits = std::stoi(argv[++i]);
        }
        else if (opt == "--lagbuffer" || opt == "-b") {
            int lagbuffer = std::stoi(argv[++i]);
            if (lagbuffer != cfg_lagbuffer_cs) {
                fprintf(stderr, "Using per-move time margin of %.2fs.\n", lagbuffer/100.0f);
                cfg_lagbuffer_cs = lagbuffer;
            }
        }
        else if (opt == "--resignpct" || opt == "-r") {
            cfg_resignpct = std::stoi(argv[++i]);
        }
        else if (opt == "--seed" || opt == "-s") {
                cfg_rng_seed = std::stoull(argv[++i]);
                if (cfg_num_threads > 1) {
                    fprintf(stderr, "Seed specified but multiple threads enabled.\n");
                    fprintf(stderr, "Games will likely not be reproducible.\n");
                }
        }
        else if (opt == "--dumbpass" || opt == "-d") {
            cfg_dumbpass = true;
        }
        else if (opt == "--weights" || opt == "-w") {
            cfg_weightsfile = argv[++i];
            players.push_back("");
        }
        else if (opt == "--logfile" || opt == "-l") {
                cfg_logfile = argv[++i];
                fprintf(stderr, "Logging to %s.\n", cfg_logfile.c_str());
                cfg_logfile_handle = fopen(cfg_logfile.c_str(), "a");
        }
        else if (opt == "--quiet" || opt == "-q") {
            cfg_quiet = true;
        }
        #ifdef USE_OPENCL
        else if (opt == "--gpu") {
            cfg_gpus = {std::stoi(argv[++i])};
        }
        #endif
        else if (opt == "--puct") {
            cfg_puct = std::stof(argv[++i]);
        }
        else if (opt == "--softmax_temp") {
            cfg_softmax_temp = std::stof(argv[++i]);
        }
        else if (opt == "--fpu_reduction") {
            cfg_fpu_reduction = std::stof(argv[++i]);
        }
        else if (opt == "--timemanage") {
            std::string tm = argv[++i];
            if (tm == "auto") {
                cfg_timemanage = TimeManagement::AUTO;
            } else if (tm == "on") {
                cfg_timemanage = TimeManagement::ON;
            } else if (tm == "off") {
                cfg_timemanage = TimeManagement::OFF;
            } else {
                fprintf(stderr, "Invalid timemanage value.\n");
                throw std::runtime_error("Invalid timemanage value.");
            }
        }
    }

    if (append_str.size())
        for (auto& line : players) {
            if (line.size())
                line += append_str;
        }

    if (cfg_timemanage == TimeManagement::AUTO) {
        cfg_timemanage = TimeManagement::ON;
    }

    if (cfg_max_playouts < std::numeric_limits<decltype(cfg_max_playouts)>::max() && cfg_allow_pondering) {
        fprintf(stderr, "Nonsensical options: Playouts are restricted but "
                            "thinking on the opponent's time is still allowed. "
                            "Ponder disabled.\n");
        cfg_allow_pondering = false;
    }

    if (players.empty()) {
        auto w = findPossibleWeightsFile(selfpath);
        if (w.size()) {
            cfg_weightsfile = w;
            players.push_back("");
        }
    }
}

