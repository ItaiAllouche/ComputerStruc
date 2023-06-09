/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
#include <vector>

//############ MACROS ############
#define TOT_REGS 8
#define BLOCK_MT 1
#define FINE_GRAINED_MT 0
#define HALT -1
#define READY 0
#define FINISHED -1
//################################

using namespace std;

//############ CLASSES ###########
class Thread{
	public:
		const int idx;
		int status;
		int pc;
		int **regs;

		//constractor
		Thread(const int idx, int* regs[TOT_REGS]);
};

class Simulator{
	public:
		const bool blockMT_mode;
		vector<Thread*> threads_vec;
		int tot_threads;
		int* regs;
		int penalty; // in case of Blocked MT
		int tot_inst;
		int tot_cycles;
		int load_latencty; // in cycles;
		int store_latencty; // in cycles;

		//constractor
		Simulator(const bool mode);

		//distractor
		~Simulator();

		// finds available thread in theads vector
		// returns thread's index 
		// otherwise retruns FINISHED
		int getAvailableThread(int curr_thread);

		// returns the idx of new thread to run or -1 if doesnt exitsts
		int contextSwitch(int curr_thread);

		// given an thread index run the next instruction
		// pre check thread's status
		void runInst(int thread_idx);

		void initVectors(void);
};
//###############################

//############ METHODS ##########
Thread::Thread(const int idx, int* regs[TOT_REGS]):idx(idx),regs(regs){
	this->status = READY;
	this-> pc = 0;
}

void Simulator::initVectors(){

	//initilaize regs vector
	for(int i = 0; i < TOT_REGS; i++){
		regs[i] = 0;
	}	

	//initilaize threads vector
	for(int i = 0; i < tot_threads; i++){
		Thread* new_pthread = new Thread(i, &regs);
		threads_vec.push_back(new_pthread);
	}
}

Simulator::Simulator(const bool mode):blockMT_mode(mode){
	this->regs = new int[TOT_REGS];
	this->tot_threads = SIM_GetThreadsNum();
	this->tot_inst = 0;
	this->tot_cycles = 0;
	this->store_latencty = SIM_GetStoreLat();
	this->load_latencty = SIM_GetLoadLat();
	this->penalty = blockMT_mode ? SIM_GetSwitchCycles() : 0;

	// initilaize threads vector and regs vector
	initVectors();
}

Simulator::~Simulator(){
	for(int i = 0; i < tot_threads; i++){
		delete threads_vec[i];
	}

	delete regs; 
}

//###############################

void CORE_BlockedMT() {
}

void CORE_FinegrainedMT() {
}

double CORE_BlockedMT_CPI(){
	return 0;
}

double CORE_FinegrainedMT_CPI(){
	return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
