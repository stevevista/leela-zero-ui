
#include "gtp_agent.h"
#include <iostream>


int main(int argc, char **argv) {

    string selfpath = argv[0];
    
    auto pos  = selfpath.rfind(
        #ifdef _WIN32
        '\\'
        #else
        '/'
        #endif
        );

    selfpath = selfpath.substr(0, pos); 


    string append;
    string cmdline;

    for (int i=1; i<argc; i++) {
        string opt = argv[i];

        if (opt == "--pass") {
            i++;
            for (; i<argc; i++) {
                append += " ";
                append += argv[i];
            }
            break;
        }

        if (opt == "--player") {
            cmdline = argv[++i];
        }
    }

    cmdline += append;
   // GameAdvisor agent1("/home/steve/dev/app/leelaz -w /home/steve/dev/data/weights.txt -g -p 1000");
   // GameAdvisor agent2("/home/steve/dev/app/leelaz -w /home/steve/dev/data/weights.txt -g -p 1000");
   // two_player(agent1, agent2);
   // return 0;

    //"/home/lc/leelaz -w /home/lc/data/weights.txt -g -p 10 --noponder"
    
    GameAdvisor agent(cmdline, selfpath);
    //GameAdvisor agent("/home/steve/dev/app/leelaz -w /home/steve/dev/data/weights.txt -g");
    
    agent.onGtpIn = [](const string& line) {
        cout << line << endl;
    };

    agent.onGtpOut = [](const string& line) {
        cout << line;
    };

    agent.onThinkMove = [&](bool black, int move) {
        agent.place(black, move);
    };

    // User player input
    std::thread([&]() {

		while(true) {
			std::string input_str;
			getline(std::cin, input_str);

            auto pos = agent.text_to_move(input_str);
            if (pos != GtpAgent::invalid_move) {
                agent.place(agent.next_move_is_black(), pos);
            }
            else
                agent.send_command(input_str);
		}

	}).detach();

    // computer play as BLACK
    agent.reset(true);

    while (true) {
        this_thread::sleep_for(chrono::microseconds(100));
        agent.pop_events();
        if (!agent.alive())
            break;
    }

    agent.join();

    return 0;
}

