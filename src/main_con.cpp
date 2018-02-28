
#include "gtp_agent.h"
#include <iostream>

int main() {

    GameAdvisor agent("/home/lc/leelaz -w /home/lc/data/weights.txt -g -p 10 --noponder");
    agent.onOutput = [](const string& line) {
        cout << line;
    };

    //agent.join();

    std::thread([&]() {

		while(true) {
			std::string input_str;
			getline(std::cin, input_str);

            agent.sendCommand(input_str, false);
		}

	}).detach();


    agent.onThinkMove = [&](bool black, int move) {
        cout << "-------------------------------" << endl;
        cout << black << endl;
        cout << move << endl;
        agent.place(false, 0);
    };

    this_thread::sleep_for(chrono::seconds(5));
    agent.think(true);
    

    while (true) {
        this_thread::sleep_for(chrono::seconds(1));
        agent.pop_events();
        //cout << agent.alive() << endl;
        if (!agent.alive())
            break;
    }

    agent.restore();

    agent.join();

    


    return 0;
}

