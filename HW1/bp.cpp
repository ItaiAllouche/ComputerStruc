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
#define EMPTY -1
#define LH_LFSM 0
#define LH_GFSM 1
#define GH_LFSM 2
#define GH_GFSM 3



using namespace std;

void fsmInitiate(char** fsm, int fsm_size){
	for(int i=0; i<fsm_size; i++){
		fsm[i] = SNT;
	}
}

void histInitiate(char history[], int history_vec_size){
	for(int i=0; i<history_vec_size; i++){
		history[i] = 0;
	}
}

Cell::Cell():
	tag(0),
	target(0){};

Cell::Cell(const Cell &cell): 
	tag(cell.tag),
	target(cell.target){};

//constructor for Btb class
Btb::Btb(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
		 bool isGlobalHist, bool isGlobalTable, int shared){
		this->btbSize = btbSize;
		this->historySize = historySize;
		this->tagSize = tagSize;
		this->isGlobalHist = isGlobalHist;
		this->isGlobalTable = isGlobalTable;
		this->shared = shared;
		this->ptr_table = new Cell[btbSize];
		this->shared_mask = 0xFFFF;;
		this->btb_mask = 0xFFFF;
		this->tag_mask = 0xFFFF;


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
			shared_mask << historySize;
			shared_mask = ~shared_mask;
			shared_mask << 2;
		}
		else if(isGlobalTable && shared == USING_SHARE_MID){
			shared_mask << historySize;
			shared_mask = ~shared_mask;
			shared_mask << 16;
		}

		//initiate btb_mask
		int num_of_bits = (int)log2((double)btbSize);
		btb_mask << num_of_bits;
		btb_mask = ~btb_mask;
		btb_mask << 2;

		//initiate tag_mask
		tag_mask << tagSize;
		tag_mask = ~tag_mask;
		tag_mask << (2 + num_of_bits);

		int history_vec_size = 1;
		int fsm_column = 1;

		//local history
		if(!isGlobalHist){
			history_vec_size = btbSize;
			this->history = new char[history_vec_size];
		}
		else{
			this->history = new char[1];
		}

		//local fsm machines
		if(!isGlobalTable){
			fsm_column = btbSize;
			int fsm_size = (int)pow(2, historySize);
			this->fsm = new char*[fsm_size];
			for(int i=0; i< fsm_size; i++)
				this->fsm[i] = new char[fsm_column];
		}

		//check if is it the right way of allocation*******************************
		else{
			this->fsm = new char*[(int)pow(2, historySize)];
		}

		//initiate fsm machines and history vector to 0
		fsmInitiate(this->fsm, (int)pow(2, historySize)*fsm_column);
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
	int tag = pc & tag_mask;



	//current branch alreday exsits in Btb
	if(ptr_table[btb_index].tag == tag){
		switch(history_fsm_state){

			//local hist & local fsm
			case LH_LFSM:
				if(taken){
					if(fsm[history[btb_index]][btb_index] != ST){
						fsm[history[btb_index]][btb_index]++;
					}
				}
				else{
					if(fsm[history[btb_index]][btb_index] != SNT){
						fsm[history[btb_index]][btb_index]--;
					}
				}	
				break;

			//local hist & gloal fsm
			case LH_GFSM:
				if(taken){
					if(shared == USING_SHARE_LSB){


					}
					else if(shared == USING_SHARE_MID){

					}
					else{
						if(fsm[history[btb_index]][0] != ST){
							fsm[history[btb_index]][0]++;
						}
						
						
						
					}
					if(fsm[history[btb_index]][0] != ST){
						fsm[history[btb_index]][0]++;
					}
				}






				else{
					if(fsm[history[btb_index]][0] != SNT){
						fsm[history[btb_index]][0]--;
					}
				}	
				history[btb_index] << 1;
				if(taken){
					history[btb_index]++;
				}
				break;

			//global hist & local fsm
			case GH_LFSM:
				if(taken){
					if(fsm[history[0]][0] != ST){
						fsm[history[0]][0]++;
					}
				}
				else{
					if(fsm[history[0]][0] != SNT){
						fsm[history[0]][0]--;
					}
				}	
				history[0] << 1;
				if(taken){
					history[0]++;
				}
				break;
			

			case GH_GFSM:

		}
		history[btb_index] << 1;
		if(taken){
			history[btb_index]++;
		}



			
	}


}











int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int Shared){
				



	return -1;
}

bool BP_predict(uint32_t pc, uint32_t *dst){
	return false;
}

void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	return;
}

void BP_GetStats(SIM_stats *curStats){
	return;
}

