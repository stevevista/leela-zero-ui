
#include "gtp_agent.h"
#include <iostream>

void two_player(GtpAgent& black, GtpAgent& white) {

    GtpAgent* me = &black;
    GtpAgent* other = &white;
    bool black_to_move = true;
    bool last_is_pass = false;

    for (;;) {
        bool ok;
        auto vtx = me->send_command_sync(black_to_move ? "genmove b" : "genmove w", ok);
        if (!ok)
            throw runtime_error("unexpect error while genmove");

        cout << (black_to_move ? "B " : "W ") << vtx << endl;

        if (vtx == "resign") {
            break;
        }

        if (vtx == "pass") {
            if (last_is_pass) 
                break;
            last_is_pass = true;
        } else
            last_is_pass = false;

        other->send_command_sync((black_to_move ? "play b " : "play w ") + vtx, ok);
        if (!ok)
            throw runtime_error("unexpect error while play");

        black_to_move = !black_to_move;
        auto tmp = me;
        me = other;
        other = tmp;
    }
}

int main() {

   // GameAdvisor agent1("/home/steve/dev/app/leelaz -w /home/steve/dev/data/weights.txt -g -p 1000");
   // GameAdvisor agent2("/home/steve/dev/app/leelaz -w /home/steve/dev/data/weights.txt -g -p 1000");
   // two_player(agent1, agent2);
   // return 0;

    GameAdvisor agent("/home/lc/leelaz -w /home/lc/data/weights.txt -g -p 10 --noponder");
    //GameAdvisor agent("/home/steve/dev/app/leelaz -w /home/steve/dev/data/weights.txt -g");
    
    agent.onGtpIn = [](const string& line) {
        cout << line << endl;;
    };

    agent.onGtpOut = [](const string& line) {
        cout << line;
    };

    agent.onThinkMove = [&](bool black, int move) {
        agent.place(black, move);
    };

    //agent.join();

    std::thread([&]() {

		while(true) {
			std::string input_str;
			getline(std::cin, input_str);

            agent.send_command(input_str);
		}

	}).detach();

    agent.reset(true);


    while (true) {
        this_thread::sleep_for(chrono::microseconds(100));
        agent.pop_events();
        if (!agent.alive())
            break;
    }

/*
    this_thread::sleep_for(chrono::seconds(5));
    agent.think(true);

    agent.restore();
*/
    agent.join();

    


    return 0;
}

