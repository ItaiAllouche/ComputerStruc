/* 046267 Computer Architecture - HW #4 */

#include "core_api.h"
#include "sim_api.h"

#include <stdio.h>
#include <iostream>
#include <ostream>
#include <vector>
#include<algorithm>

//############ MACROS ############
#define BLOCKED_MT 1
#define FINE_GRAINED_MT 0
#define HALT -1
#define READY 0
#define NOT_READY -1
//################################

//######### DEBUG MACROS #########
// #define DEBUG_PRINT 1
//################################

using namespace std;

//############ CLASSES ###########
class Thread{
	public:
		const int idx;
		int status;
		int line;
		int regs[REGS_COUNT];

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

		// in blocked mode: returns NULL and increases
		// tot_cycles by penalty if sim has'nt finished
		// in fine grained mode: returns new thread to run (if exists)
		// otherwise returns current runnig thread
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
	for(int i = 0; i < REGS_COUNT; i++){
		regs[i] = 0;
	}	
}

void Simulator::initVectors(){

	//initilaize threads vector
	for(int i = 0; i < tot_threads; i++){
		Thread* new_thread = new Thread(i);
		threads_vec.push_back(new_thread);
	}
}

Simulator::Simulator(const bool blockMT_mode):blockMT_mode(blockMT_mode){
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
}

Thread* Simulator::getReadyThread(Thread* runnig_thread){

	int idx = (runnig_thread->idx + 1) % tot_threads;
	int last_idx = runnig_thread->idx;
	while(idx != last_idx){
		// current thread is ready and is not currently runnig  
		if(threads_vec[idx]->status == READY){
			return threads_vec[idx];
		}
		idx ++;
		idx %= tot_threads;
	}
	return NULL;
}

Thread* Simulator::contextSwitch(Thread* runnig_thread){
	if(blockMT_mode){

		// simulation hasnt finished
		if(tot_halts != tot_threads){
			for(int i = 0; i < penalty; i++){
				simUpdate();
			}
		}
	}

	Thread* ready_thread = getReadyThread(runnig_thread);

	if(ready_thread != NULL){
		#ifdef DEBUG_PRINT
		printf("			Context swtich %d->%d\n", runnig_thread->idx, ready_thread->idx);
		#endif			
		return ready_thread;
	}
	else{
		#ifdef DEBUG_PRINT
		printf("			Context swtich %d->%d\n", runnig_thread->idx, runnig_thread->idx);
		#endif			
		return runnig_thread;
	}

}

void Simulator::runInst(Thread* thread){
	Instruction inst;
	SIM_MemInstRead(thread->line, &inst, thread->idx);
	tot_inst ++;

	switch(inst.opcode){
		case CMD_NOP:		
			#ifdef DEBUG_PRINT
			printf("|Cycle %d|Thread %d|Command NOP|\n",tot_cycles, thread->idx);
			#endif		
			break;

		case CMD_ADD:
			#ifdef DEBUG_PRINT
			printf("|Cycle %d|Thread %d|Command ADD|\n",tot_cycles, thread->idx);
			#endif				
			thread->regs[inst.dst_index] = thread->regs[inst.src1_index] +  thread->regs[inst.src2_index_imm];
			break;


		case CMD_SUB:
			#ifdef DEBUG_PRINT
			printf("|Cycle %d|Thread %d|Command SUB|\n",tot_cycles, thread->idx);
			#endif				
			thread->regs[inst.dst_index] = thread->regs[inst.src1_index] -  thread->regs[inst.src2_index_imm];
			break;

		case CMD_ADDI:
			#ifdef DEBUG_PRINT
			printf("|Cycle %d|Thread %d|Command ADDI|\n",tot_cycles, thread->idx);
			#endif				
			thread->regs[inst.dst_index] = thread->regs[inst.src1_index] +  inst.src2_index_imm;
			break;

		case CMD_SUBI:
			#ifdef DEBUG_PRINT
			printf("|Cycle %d|Thread %d|Command SUBI|\n",tot_cycles, thread->idx);
			#endif				
			thread->regs[inst.dst_index] = thread->regs[inst.src1_index] - inst.src2_index_imm;
			break;		

		case CMD_LOAD:
			#ifdef DEBUG_PRINT
			printf("|Cycle %d|Thread %d|Command LOAD|\n",tot_cycles, thread->idx);
			#endif				
			thread->status = 1 + load_latencty;
			if(inst.isSrc2Imm){
				SIM_MemDataRead(thread->regs[inst.src1_index] + inst.src2_index_imm, &thread->regs[inst.dst_index]);
			}
			else{
				SIM_MemDataRead(thread->regs[inst.src1_index] + thread->regs[inst.src2_index_imm], &thread->regs[inst.dst_index]);
			}
			break;		

		case CMD_STORE:
			#ifdef DEBUG_PRINT
			printf("|Cycle %d|Thread %d|Command STORE|\n",tot_cycles, thread->idx);
			#endif				
			thread->status = 1 + store_latencty;
			if(inst.isSrc2Imm){				
				SIM_MemDataWrite(thread->regs[inst.dst_index] + inst.src2_index_imm, thread->regs[inst.src1_index]);
			}
			else{
				SIM_MemDataWrite(thread->regs[inst.dst_index] + thread->regs[inst.src2_index_imm], thread->regs[inst.src1_index]);
			}
			break;		

		case CMD_HALT:
			#ifdef DEBUG_PRINT
			printf("|Cycle %d|Thread %d|Command HALT|\n",tot_cycles, thread->idx);
			#endif				
			thread->status = HALT;
			tot_halts ++;
			break;
	}
	thread->line ++;
}

void Simulator::simUpdate(){

	//update waiting thread's countdown on each cycle
	for(int i = 0; i < tot_threads; i++){
		if(threads_vec[i]->status > 0){
			threads_vec[i]->status--;
		}
	}

	// increment total number of cycles by 1 on each cycle
	tot_cycles++;
}

void copyArray(tcontext* dst, int* src, int len, int threadid){
	for(int i = 0; i < len; i++){
		dst[threadid].reg[i] = src[i];
	}
	return;
}
//###############################

Simulator* blocked_sim = NULL;
Simulator* fine_grained_sim = NULL;

void CORE_BlockedMT(){
	blocked_sim = new Simulator(BLOCKED_MT);

	//first thread to run
	Thread* runnig_thread = blocked_sim->threads_vec[0];	
	while(blocked_sim->tot_halts != blocked_sim->tot_threads){
		while(runnig_thread->status == READY){
			blocked_sim->runInst(runnig_thread);
			blocked_sim->simUpdate();
		}

		// current running thread cannot run in current cycle
		// finds other thread (if exsits) to run
		// if found -> context switch
		if(blocked_sim->getReadyThread(runnig_thread) != NULL){
			runnig_thread = blocked_sim->contextSwitch(runnig_thread);
		}
		else if(blocked_sim->tot_halts == blocked_sim->tot_threads){
			break;
		}
		else{
			#ifdef DEBUG_PRINT
			printf("			idle\n");			
			#endif
			blocked_sim->simUpdate();			
		}
	}		

	#ifdef DEBUG_PRINT
	printf("\n|CYCLES %d|INSTRUCTIONS %d\n", blocked_sim->tot_cycles, blocked_sim->tot_inst);
	#endif
}

void CORE_FinegrainedMT(){
	fine_grained_sim = new Simulator(FINE_GRAINED_MT);

	//first thread to run
	Thread* runnig_thread = fine_grained_sim->threads_vec[0];
	while(fine_grained_sim->tot_halts != fine_grained_sim->tot_threads){
		if(runnig_thread->status == READY){
			fine_grained_sim->runInst(runnig_thread);
		}

		//cycle = next cycle
		fine_grained_sim->simUpdate();
		runnig_thread = fine_grained_sim->contextSwitch(runnig_thread);
	}

	#ifdef DEBUG_PRINT
	printf("\n|CYCLES %d|INSTRUCTIONS %d\n", fine_grained_sim->tot_cycles, fine_grained_sim->tot_inst);
	#endif	
}

double CORE_BlockedMT_CPI(){
	double tot_inst = blocked_sim->tot_inst;
	double tot_cycles = blocked_sim->tot_cycles;
	delete blocked_sim;
	
	if(tot_inst == 0){
		return -1;
	}
	
	return tot_cycles / tot_inst;
}

double CORE_FinegrainedMT_CPI(){
	double tot_inst = fine_grained_sim->tot_inst;
	double tot_cycles = fine_grained_sim->tot_cycles;
	delete fine_grained_sim;
	
	if(tot_inst == 0){
		return -1;
	}
	
	return tot_cycles / tot_inst;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid){
	//copy_n(blocked_sim->threads_vec[threadid]->regs, REGS_COUNT, context->reg);
	copyArray(context, blocked_sim->threads_vec[threadid]->regs, REGS_COUNT, threadid);
	return;
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid){
	copyArray(context, fine_grained_sim->threads_vec[threadid]->regs, REGS_COUNT, threadid);
	return;	
}
