
#include "board.h"
#include "playout.h"
#include <fstream>
#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <signal.h>

#ifndef _WIN32
	#include <unistd.h>
	#include <sys/wait.h>
#endif

using namespace std;

void ReadConfiguration(int argc, char **argv);

int main(int argc, char **argv) {

	ReadConfiguration(argc, argv);

	std::ofstream ofs("score.data", std::ofstream::binary);

	int count = 0;

	for (;;) {
		auto data = generate_sample();
		if (!data.empty()) {
			ofs.write((char*)&data[0], data.size());
			count++;
			if (count % 100 == 0)
				std::cout << "write: " <<  count << std::endl;
			if (count >= 100) break;
		}
	}
	ofs.close();
	return 0;

}

void ReadConfiguration(int argc, char **argv){

	// Get working directory path.
	char buf[1024] = {};
#ifdef _WIN32
	GetModuleFileName(NULL, buf, sizeof(buf)-1);
	std::string split_str = "\\";
#else
	readlink("/proc/self/exe", buf, sizeof(buf)-1);
	std::string split_str = "/";
#endif
	std::string path_(buf);
	// Delete file name.
	auto pos_filename = path_.rfind(split_str);
	if(pos_filename != std::string::npos){
		working_dir = path_.substr(0, pos_filename+1);
	}

	ImportProbDist();
	ImportProbPtn3x3();

	std::cerr << "configuration loaded.\n";
}
