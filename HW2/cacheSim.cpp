/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include "cache.h"
#include <stdio.h>
#include <string.h>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

int main(int argc, char **argv) {

	if (argc < 19) {
		cout << argc << endl;
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	Cache* cache_lv2 = new Cache(2, NULL, L2Size, L2Cyc, BSize, WrAlloc, L2Assoc);
	Cache* cache_lv1 = new Cache (1, cache_lv2, L1Size, L1Cyc, BSize, WrAlloc, L1Assoc);
	
	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		string operation; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		
		uint pc = stoi(cutAddress, 0, HEX);
		unsigned block = cache_lv1->getBlock(pc);
		printf("relevent pc is 0x%x\n", pc);
		// printf("relevent block is 0x%x\n", block);

		char op;
		if(operation == "w"){
			op = WRITE;
		}
		else{
			op = READ;
		}
		cache_lv1->update(block, op);
	}

	printf("lvl1 tot miss are:%f\n", cache_lv1->tot_miss);
	printf("lvl1 tot hits are:%f\n", cache_lv1->tot_hits);
	printf("lvl1 tot access are:%f\n", cache_lv1->cache_access);
	printf("lvl2 tot miss are:%f\n", cache_lv2->tot_miss);
	printf("lvl2 tot hits are:%f\n", cache_lv2->tot_hits);
	printf("lvl2 tot access are:%f\n", cache_lv2->cache_access);

	double L1MissRate = cache_lv1->tot_miss / cache_lv1->cache_access;
	double L2MissRate = cache_lv2->tot_miss / cache_lv2->cache_access;
	double avgAccTime =  (1-L1MissRate)*L1Cyc + //lv1 hit
						 L1MissRate*(1-L2MissRate)*(L1Cyc + L2Cyc) + // lv1 miss lv2 hit 
						 L1MissRate*L2MissRate*(L1Cyc + L2Cyc + MemCyc); //lv1 miss lv2 miss

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	
	return 0;
}
