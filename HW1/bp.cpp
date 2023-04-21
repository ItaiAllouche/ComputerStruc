/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"

//4 FSM states
#define SNT 0x00
#define WNT 0x01
#define WT 0x10
#define ST 0x11

#define USING_SHARE_LSB 0
#define USING_SHARE_MID 1
#define NOT_USING_SHARE 2
#define EMPTY -1
#define LH_LFSM 0
#define LH_GFSM 1
#define GH_LFSM 2
#define GH_GFSM 3
#define ADDRESS_LENGTH 32

using namespace std;

Btb* btb;

void fsmInitiate(char** fsm, int fsm_size, int current_fsm, int unsigned fsmState){
	for(int i = 0; i < fsm_size; i++){
		fsm[i][current_fsm] = fsmState;
	}
}

void histInitiate(char history[], int history_vec_size){
	for(int i=0; i<history_vec_size; i++){
		history[i] = 0;
	}
}

Cell::Cell(){
	this->pc = 0;
	this->tag = 0;
	this->target = 0;
};

// Cell::Cell(const Cell &cell):
// 	pc(cell.pc),
// 	tag(cell.tag),
// 	target(cell.target){};

//constructor for Btb class
Btb::Btb(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
		 bool isGlobalHist, bool isGlobalTable, int shared){
		this->btbSize = btbSize;
		this->historySize = historySize;
		this->tagSize = tagSize;
		this->fsmState = fsmState;
		this->isGlobalHist = isGlobalHist;
		this->isGlobalTable = isGlobalTable;
		this->shared = shared;
		this->ptr_table = new Cell[btbSize];
		this->shared_mask = 0xFFFF;;
		this->btb_mask = 0xFFFF;
		this->tag_mask = 0xFFFF;
		this->stats = 0;


		if(!isGlobalHist && !isGlobalTable)
			this->history_fsm_state = LH_LFSM;

		else if(!isGlobalHist && isGlobalTable)
			this->history_fsm_state = LH_GFSM;

		else if(isGlobalHist && !isGlobalTable)
			this->history_fsm_state = GH_LFSM;

		else 
			this->history_fsm_state = GH_GFSM;

		//initate shared_mask if needed
		if(isGlobalTable && shared == USING_SHARE_LSB){
			shared_mask <<= historySize;
			shared_mask = ~shared_mask;
			 shared_mask <<= 2;
		}
		else if(isGlobalTable && shared == USING_SHARE_MID){
			shared_mask <<= historySize;
			shared_mask = ~shared_mask;
			shared_mask <<= 16;
		}

		//initiate btb_mask
		int num_of_bits = (int)log2((double)btbSize);
		btb_mask <<= num_of_bits;
		//btb_mask << btbSize;
		btb_mask = ~btb_mask;
		btb_mask <<= 2;

		//initiate tag_mask
		tag_mask <<= tagSize;
		tag_mask = ~tag_mask;
		tag_mask <<= (2 + num_of_bits);

		int history_vec_size = isGlobalHist ? 1 : btbSize;
		int fsm_size = 1;
		this->history = new char[history_vec_size];

		//local fsm machines
		if(!isGlobalTable){;
			fsm_size = (int)pow(2, historySize);
			this->fsm = new char*[fsm_size];
			for(int i=0; i < fsm_size; i++){
				this->fsm[i] = new char[btbSize];
			}

		}

		//check if is it the right way of allocation*******************************
		else{
			this->fsm = new char*[(int)pow(2, historySize)];
		}

		//initiate fsm machines and history vector
		for(unsigned current_fsm=0; current_fsm < btbSize; current_fsm++){
			fsmInitiate(this->fsm, current_fsm, fsm_size, fsmState);
		}
		histInitiate(this->history, history_vec_size);
	}

//distractor for Btb class
Btb::~Btb(){
	delete this->ptr_table;
	delete this->history;
	delete this->fsm;
}

void Btb::update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	int btb_index = pc & btb_mask;
	unsigned tag = pc & tag_mask;
	btb_index >>= 2;
	tag >>= (2 + btbSize);

	//current branch alreday exsits in Btb
	if(ptr_table[btb_index].tag == tag){

		//updating fsm and target if neccesery
		switch(history_fsm_state){

			//local hist & local fsm
			case LH_LFSM:
				if(taken){

					//flush routine
					if(fsm[(int)history[btb_index]][btb_index]  < 0x10 || pred_dst != targetPc){
						ptr_table[btb_index].target = targetPc;
						stats->flush_num ++;
					}

					//update fsm 
					if(fsm[(int)history[btb_index]][btb_index] != ST){
						fsm[(int)history[btb_index]][btb_index]++;
					}
				}
				else{
					//flush routine
					if(fsm[(int)history[btb_index]][btb_index] > 0x1 || pred_dst != targetPc){
						stats->flush_num ++;
					}

					if(fsm[(int)history[btb_index]][btb_index] != SNT){
						fsm[(int)history[btb_index]][btb_index]--;
					}
				}	
				break;

			//global hist & local fsm
			case GH_LFSM:
				if(taken){

					//flush routine
					if((int)fsm[(int)history[btb_index]][btb_index]  < 0x10 || pred_dst != targetPc){
						ptr_table[btb_index].target = targetPc;
						stats->flush_num ++;
					}

					if(fsm[(int)history[0]][btb_index] != ST){
						fsm[(int)history[0]][btb_index]++;
					}
				}
				else{
					//flush routine
					if(fsm[(int)history[btb_index]][btb_index] > 0x1 || pred_dst != targetPc){
						stats->flush_num ++;
					}

					if(fsm[(int)history[0]][btb_index] != SNT){
						fsm[(int)history[0]][btb_index]--;
					}
				}				
				break;

			//local hist & gloal fsm
			case LH_GFSM:
				if(taken){
					if(shared == USING_SHARE_LSB || shared == USING_SHARE_MID){
						int fsm_index = ptr_table[btb_index].pc & shared_mask;	
						if(shared == USING_SHARE_LSB){
							fsm_index >>= 2;
						}
						else{
							fsm_index >>= 16;
						}

						fsm_index ^= history[btb_index];

						//flush ruotine
						if(fsm[fsm_index][0] < 0x10 || pred_dst != targetPc){
							ptr_table[btb_index].target = targetPc;
							stats->flush_num ++;
						}

						if(fsm[fsm_index][0] != ST){
							fsm[fsm_index][0]++;
						}
					}
					else{
						//flush ruotine
						if(fsm[(int)history[btb_index]][0] < 0x10 || pred_dst != targetPc){
							ptr_table[btb_index].target = targetPc;
							stats->flush_num ++;
						}

						if(fsm[(int)history[btb_index]][0] != ST){
							fsm[(int)history[btb_index]][0]++;
						}	
					};
				}
				
				else{
					if(shared == USING_SHARE_LSB || shared == USING_SHARE_MID){
						int fsm_index = ptr_table[btb_index].pc & shared_mask;	
						if(shared == USING_SHARE_LSB){
							fsm_index >>= 2;
						}
						else{
							fsm_index >>= 16;
						}

						fsm_index ^= history[btb_index];

						//flush routine
						if(fsm[fsm_index][0] > 0x1 || pred_dst != targetPc){
							ptr_table[btb_index].target = targetPc;
							stats->flush_num ++;
						}
						if(fsm[fsm_index][0] != SNT){
							fsm[fsm_index][0]--;
						}
					}

					else{
						//flush routine
						if(fsm[(int)history[btb_index]][0] > 0x1 || pred_dst != targetPc){
							ptr_table[btb_index].target = targetPc;
							stats->flush_num ++;
						}

						if(fsm[(int)history[btb_index]][0] != SNT){
							fsm[(int)history[btb_index]][0]--;
						}	
					}
				}
				break;

			//global hist & gloal fsm
			case GH_GFSM:
				if(taken){
					if(shared == USING_SHARE_LSB || shared == USING_SHARE_MID){
						int fsm_index = ptr_table[btb_index].pc & shared_mask;	
						if(shared == USING_SHARE_LSB){
							fsm_index >>= 2;
						}
						else{
							fsm_index >>= 16;
						}

						fsm_index ^= history[0];

						//flush routine
						if(fsm[fsm_index][0]  < 0x10 || pred_dst != targetPc){
							ptr_table[btb_index].target = targetPc;
							stats->flush_num ++;
						}	

						if(fsm[fsm_index][0] != ST){
							fsm[fsm_index][0]++;
						}
					}
					else{
						//flush routine
						if(fsm[(int)history[0]][0]  < 0x10 || pred_dst != targetPc){
							ptr_table[btb_index].target = targetPc;
							stats->flush_num ++;
						}

						if(fsm[(int)history[0]][0] != ST){
							fsm[(int)history[0]][0]++;
						}	
					}
				}
				else{
					if(shared == USING_SHARE_LSB || shared == USING_SHARE_MID){
						int fsm_index = ptr_table[btb_index].pc & shared_mask;	
						if(shared == USING_SHARE_LSB){
							fsm_index >>= 2;
						}
						else{
							fsm_index >>= 16;
						}

						fsm_index ^= history[0];

						//flush routine
						if(fsm[fsm_index][0] > 0x1 || pred_dst != targetPc){
							ptr_table[btb_index].target = targetPc;
							stats->flush_num ++;
						}

						if(fsm[fsm_index][0] != SNT){
							fsm[fsm_index][0]--;
						}
					}
					else{

						//flush routine
						if(fsm[(int)history[0]][0] > 0x1 || pred_dst != targetPc){
							ptr_table[btb_index].target = targetPc;
							stats->flush_num ++;
						}

						if(fsm[(int)history[0]][0] != SNT){
							fsm[(int)history[0]][0]--;
						}	
					}
				}
				break;
		}//finish updating fsm

		//updating history vector
		if(history_fsm_state > 0x1){
			history[0] <<= 1;
			if(taken){
				history[0]++;
			}
		}
		else{
			history[btb_index] <<= 1;
			if(taken){
				history[btb_index]++;
			}
		}//finish updating history vector
	}
	
	//cuurent branch doesnt exists in Btb
	else{
		ptr_table[btb_index].pc = pc; 
		ptr_table[btb_index].tag = (pc & tag_mask) >> (2 + btbSize);
		ptr_table[btb_index].target = targetPc;

		//local hist
		if(history_fsm_state == LH_GFSM || history_fsm_state == LH_LFSM){
			history[btb_index] = 0;
		}

		//local fsm
		if(history_fsm_state == LH_LFSM || history_fsm_state == GH_LFSM){
			int fsm_size = (int)pow(2, historySize);
			fsmInitiate(fsm, fsm_size, btb_index, fsmState);
		}
	}

	//update stats
	stats->br_num++;
	return;
}

bool Btb::predict(uint32_t pc, uint32_t *dst){
	int btb_index = pc & btb_mask;
	unsigned tag = pc & tag_mask;
	btb_index >>= 2;
	tag >>= (2 + btbSize);

	//current branch doesnt exists in Btb -> predict not taken
	if(ptr_table[btb_index].tag != tag){
		*dst = pc + 4;
		return false;
	}

	int history_column = isGlobalHist ? 0 : btb_index; 
	int fsm_column = isGlobalTable ? 0 : btb_index;
	int prediction;

	if(isGlobalTable && shared != NOT_USING_SHARE){
		int fsm_index = pc & shared_mask;	
		if(shared == USING_SHARE_LSB){
			fsm_index >>= 2;
		}
		else{
			fsm_index >>= 16;
		}

		fsm_index ^= history[history_column];
		prediction = fsm[fsm_index][fsm_column];	
	}
	else{
		prediction = fsm[(int)history[history_column]][fsm_column];	
	}
	
	
	//prdict taken
	if(prediction == ST || prediction == WT){
		*dst = ptr_table[btb_index].target;
		return true;
	}

	//predict not taken
	else{
		*dst = pc + 4;
		return false;
	}
}

void Btb::getStats(SIM_stats *curStats){

	//calc predictior size [Bits]
	stats->size = btbSize*(tagSize + ADDRESS_LENGTH);

	//memory size from history vector
	if(isGlobalHist) {
		stats->size += historySize;
	}
	else{
		stats->size += btbSize*historySize;
	}

	//memory size from fsm
	if(isGlobalTable){
		stats->size += (int)pow(2, historySize);
	}
	else{
		stats->size += btbSize*(int)pow(2, historySize);
	}

	curStats = this->stats;
	return;
}


int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int shared){

	*btb = Btb(btbSize, historySize, tagSize, fsmState,
		isGlobalHist, isGlobalTable, shared);			
	return 0;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return btb->predict(pc, dst);
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	btb->update(pc, targetPc, taken, pred_dst);
	return;
}

void BP_GetStats(SIM_stats *curStats){
	btb->getStats(curStats);
	return;
}

