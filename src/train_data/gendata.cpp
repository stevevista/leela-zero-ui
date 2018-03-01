


#include "archive.hpp"
#include <algorithm>

static std::string opt_input = "../../data/sgfs/train";
static std::string opt_output = "../../data/train.data";
static int opt_size = 19;
static bool opt_verify = false;

void parse_commandline( int argc, char **argv ) {
    for (int i = 1; i < argc; i++) { 
        std::string opt = argv[i];

        if (opt == "--input" || opt == "-i") {
            opt_input = argv[++i];
        } else if (opt == "--output" || opt == "-o") {
            opt_output = argv[++i];
        } else if (opt == "--size") {
            opt_size = std::stoi(argv[++i]);
        } else if (opt == "--verify") {
            opt_verify = true;
        } 
    }
}

int main(int argc, char **argv) {

    parse_commandline(argc, argv);

    if (opt_verify) {
        auto total = GameArchive::verify(opt_input);
        std::cout << "total_moves: " << total << std::endl;
        return 0;
    }
    GameArchive::generate(opt_input, opt_output, opt_size);
    return 0;
}
