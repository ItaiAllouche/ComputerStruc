/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
#include <vector>

//############ MACROS ############
#define TOT_REGS 8
#define BLOCKED_MT 1
#define FINE_GRAINED_MT 0
#define HALT -1
#define READY 0
#define NOT_READY -1
//################################

using namespace std;

//############ CLASSES ###########
class Thread{
	public:
		const int idx;
		int status;
		int line;
		int* regs;

		//constractor
		Thread(const int idx);

		// initilaize regs to 0
		void initRegs(void);
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
		int tot_halts;
		int load_latencty; // in cycles;
		int store_latencty; // in cycles;

		//constractor
		Simulator(const bool blockMT_mode);

		//distractor
		~Simulator();

		// finds ready thread in theads vector
		// returns new ready thread if exists 
		// otherwise returns NULL
		Thread* getReadyThread(Thread* runnig_thread);

		// returns  new/same thread to run or NULL if doesnt exists
		Thread* contextSwitch(Thread* runnig_thread);

		// given thread run it's next instruction
		// pre check thread's status
		void runInst(Thread* thread);

		void initVectors(void);

		void simUpdate(void);
};
//###############################

//############ METHODS ##########
Thread::Thread(const int idx):idx(idx){
	this->status = READY;
	this-> line = 0;
	this->initRegs();
}

void Thread::initRegs(void){
	for(int i = 0; i < TOT_REGS; i++){
		regs[i] = 0;
	}	
}

void Simulator::initVectors(){

	//initilaize regs vector
	for(int i = 0; i < TOT_REGS; i++){
		regs[i] = 0;
	}	

	//initilaize threads vector
	for(int i = 0; i < tot_threads; i++){
		Thread* new_thread = new Thread(i);
		threads_vec.push_back(new_thread);
	}
}

Simulator::Simulator(const bool blockMT_mode):blockMT_mode(blockMT_mode){
	this->regs = new int[TOT_REGS];
	this->tot_threads = SIM_GetThreadsNum();
	this->tot_inst = 0;
	this->tot_cycles = 0;
	this->tot_halts = 0;
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

Thread* Simulator::getReadyThread(Thread* runnig_thread){
	for(int i = 0; i < tot_threads; i++){

		// current thread is ready and is not currently runnig  
		if((threads_vec[i]->idx != runnig_thread->idx) && (threads_vec[i]->status == READY)){
			return threads_vec[i];
		}
	}
	return NULL;
}

Thread* Simulator::contextSwitch(Thread* runnig_thread){
	if(blockMT_mode){
		if(tot_halts != tot_threads){
			tot_cycles += penalty;
		}
		return NULL;
	}
	else{
		Thread* ready_thread = getReadyThread(runnig_thread);

		if(ready_thread != NULL){
			return ready_thread;
		}
		else if(runnig_thread->status == READY){
			return runnig_thread;
		}
		else{
			return NULL;
		}
	}

}

void Simulator::runInst(Thread* thread){
	Instruction inst;
	SIM_MemInstRead(thread->line, &inst, thread->idx);

	switch(inst.opcode){
		case CMD_NOP:
			break;

		case CMD_ADD:
			regs[inst.dst_index] = regs[inst.src1_index] +  regs[inst.src2_index_imm];
			break;


		case CMD_SUB:
			regs[inst.dst_index] = regs[inst.src1_index] -  regs[inst.src2_index_imm];
			break;

		case CMD_ADDI:
			regs[inst.dst_index] = regs[inst.src1_index] +  inst.src2_index_imm;
			break;

		case CMD_SUBI:
			regs[inst.dst_index] = regs[inst.src1_index] - inst.src2_index_imm;
			break;		

		case CMD_LOAD:
			thread->status = load_latencty;
			if(inst.isSrc2Imm){
				SIM_MemDataRead(regs[inst.src1_index] + inst.src2_index_imm, &regs[inst.dst_index]);
			}
			else{
				SIM_MemDataRead(regs[inst.src1_index] + regs[inst.src2_index_imm], &regs[inst.dst_index]);
			}
			break;		

		case CMD_STORE:
			thread->status = store_latencty;
			if(inst.isSrc2Imm){				
				SIM_MemDataWrite(regs[inst.src1_index] + inst.src2_index_imm, regs[inst.src1_index]);
			}
			else{
				SIM_MemDataWrite(regs[inst.src1_index] + regs[inst.src2_index_imm], regs[inst.src1_index]);
			}
			break;		

		case CMD_HALT:
			thread->status = HALT;
			tot_halts ++;
			break;
	}
	thread->line ++;
}

void Simulator::simUpdate(){

	//update waiting thread's countdown
	for(int i = 0; i < tot_threads; i++){
		if(threads_vec[i]->status > 0){
			threads_vec[i]->status--;
		}
	}

	// increment total number of cycles by 1 on each cycle
	tot_cycles++;
}

//###############################

Simulator* blocked_sim = NULL;
Simulator* fine_grained_sim = NULL;

void CORE_BlockedMT() {
	blocked_sim = new Simulator(BLOCKED_MT);
	for(int idx = 0; idx < blocked_sim->tot_threads; idx++){
		while(sim->threads_vec[idx]->status != HALT){
			blocked_sim->runInst(blocked_sim->threads_vec[idx]);
			blocked_sim->simUpdate();
		}
		blocked_sim->contextSwitch(blocked_sim->threads_vec[idx]);
	}
}

void CORE_FinegrainedMT() {
	fine_grained_sim = new Simulator(FINE_GRAINED_MT);

	//first thread to run
	Thread* runnig_thread = fine_grained_sim->threads_vec[0];
	while(fine_grained_sim->tot_halts != fine_grained_sim->tot_threads){
		fine_grained_sim->runInst(runnig_thread);
		fine_grained_sim->simUpdate();
		runnig_thread = fine_grained_sim->contextSwitch(runnig_thread);
	}
}

double CORE_BlockedMT_CPI(){
	return 0;
}

double CORE_FinegrainedMT_CPI(){
	return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	context = blocked_sim->regs;
	return;
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
