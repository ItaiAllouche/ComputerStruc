/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <math.h>
#include <cstdio>

//4 FSM states
#define SNT 0
#define WNT 1
#define WT 2
#define ST 3

#define NOT_USING_SHARE 0
#define USING_SHARE_LSB 1
#define USING_SHARE_MID 2
#define EMPTY -1
#define LH_LFSM 0
#define LH_GFSM 1
#define GH_LFSM 2
#define GH_GFSM 3
#define ADDRESS_LENGTH 30
#define VALID_BIT 1

using namespace std;



//Cell class for cell in BTB
class Cell{
public:
	unsigned int tag;
	unsigned int target;

	Cell();
	Cell (const Cell &cell);
};

//Btb calss
class Btb{
	unsigned btbSize;
	unsigned historySize;
	unsigned tagSize;
	unsigned fsmState;
	bool isGlobalHist;
	bool isGlobalTable;
	int shared;
	Cell* ptr_table;
	char* history;
	char** fsm;
	char history_fsm_state;	//LH_LFSM || LH_GFSM || GH_LFSM || GH_GFSM
	unsigned shared_mask;
	unsigned btb_mask;
	unsigned tag_mask;
	unsigned hist_mask;
	int num_of_bits;
	SIM_stats* stats;

public:
	Btb(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int shared);

	~Btb();

	bool predict(uint32_t pc, uint32_t *dst);

	void update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);

	void getStats(SIM_stats *curStats);
};

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

//initiate all tag values in table to -1
void tableInitiate(Cell* ptr_table, unsigned btbSize){
	for(unsigned i=0; i<btbSize; i++){
		ptr_table[i].tag = -1;
	}
}

Cell::Cell(){
	this->tag = 0;
	this->target = 0;
};

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
		this->shared_mask = 0xFFFFFFFF;
		this->btb_mask = 0xFFFFFFFF;
		this->tag_mask = 0xFFFFFFFF;
		this->hist_mask = 0xFFFFFFFF;
		this->stats = new SIM_stats;
		this->stats->flush_num = 0;
		this->stats->br_num = 0;
		this->stats->size = 0;
		int num_of_bits = (int)log2((double)btbSize);

		if(!isGlobalHist && !isGlobalTable)
			this->history_fsm_state = LH_LFSM;

		else if(!isGlobalHist && isGlobalTable)
			this->history_fsm_state = LH_GFSM;

		else if(isGlobalHist && !isGlobalTable)
			this->history_fsm_state = GH_LFSM;

		else 
			this->history_fsm_state = GH_GFSM;

		//initiate ptr_table tag values to -1
		tableInitiate(ptr_table, btbSize);

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
		btb_mask <<= num_of_bits;
		btb_mask = ~btb_mask;
		btb_mask <<= 2;

		//initiate tag_mask
		tag_mask <<= tagSize;
		tag_mask = ~tag_mask;
		tag_mask <<= (2 + num_of_bits);

		//initiate hist_mask
		hist_mask <<= historySize;
		hist_mask = ~hist_mask;
		
		int fsm_size = (int)pow(2, historySize);
		int fsm_columns = isGlobalTable ? 1 : btbSize;
		int history_vec_size = isGlobalHist ? 1 : btbSize;

		//allocate history vector
		this->history = new char[history_vec_size];

		//allocate fsms table
		this->fsm = new char*[fsm_size];
		for(int i=0; i < fsm_size; i++){
			this->fsm[i] = new char[fsm_columns];
		}

		//initiate fsm machines
		if(!isGlobalTable){
			for(unsigned current_fsm = 0; current_fsm < btbSize; current_fsm++){
				fsmInitiate(this->fsm, fsm_size, current_fsm, fsmState);
			}
		}
		else{
			fsmInitiate(this->fsm, fsm_size, 0, fsmState);
			}

		//initiate history vector
		histInitiate(this->history, history_vec_size);
	}

//distractor for Btb class
Btb::~Btb(){
	delete[] this->ptr_table;
	delete[] this->history;

	int fsm_size = (int)pow(2, historySize);

	//deallocation of fsm, according to global/local fsm
	if(!isGlobalTable && fsm != NULL){
		for(int i=0; i < fsm_size; i++){
			delete[] this->fsm[i];
		}
		delete[] this->fsm;
	}
	else if(isGlobalTable && fsm != NULL){
		delete[] this->fsm[0];
		delete[] this->fsm;
	}

	delete this->stats;
}

void Btb::update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	int btb_index = pc & btb_mask;
	int fsm_row;
	unsigned tag = pc & tag_mask;
	btb_index >>= 2;
	tag >>= (2 + num_of_bits);

	//current branch alreday exsits in Btb
	if(ptr_table[btb_index].tag == tag){

		//updating fsm and target
		switch(history_fsm_state){

			//local hist & local fsm
			case LH_LFSM:
				fsm_row = (int)(history[btb_index] & hist_mask);
				if(taken){

					//flush routine
					if(fsm[fsm_row][btb_index] < 2 || pred_dst != targetPc){
						stats->flush_num ++;
					}

					//update fsm 
					if(fsm[fsm_row][btb_index] != ST){
						fsm[fsm_row][btb_index]++;
					}
				}
				else{
					//flush routine
					if(fsm[fsm_row][btb_index] > 1){
						stats->flush_num ++;
					}

					if(fsm[fsm_row][btb_index] != SNT){
						fsm[fsm_row][btb_index]--;
					}
				}	
				break;

			//global hist & local fsm
			case GH_LFSM:
				fsm_row = (int)(history[0] & hist_mask);
				if(taken){

					//flush routine
					if(fsm[fsm_row][btb_index] < 2 || pred_dst != targetPc){
						stats->flush_num ++;
					}

					if(fsm[fsm_row][btb_index] != ST){
						fsm[fsm_row][btb_index]++;
					}
				}
				else{
					//flush routine
					if(fsm[fsm_row][btb_index] > 1){
						stats->flush_num ++;
					}

					if(fsm[fsm_row][btb_index] != SNT){
						fsm[fsm_row][btb_index]--;
					}
				}				
				break;

			//local hist & gloal fsm
			case LH_GFSM:
				if(taken){
					if(shared == USING_SHARE_LSB || shared == USING_SHARE_MID){
						fsm_row = pc & shared_mask;
						if(shared == USING_SHARE_LSB){
							fsm_row >>= 2;
						}
						else{
							fsm_row >>= 16;
						}

						fsm_row ^= (int)(history[btb_index] & hist_mask);

						//flush routine
						if(fsm[fsm_row][0] < 2 || pred_dst != targetPc){
							stats->flush_num ++;
						}

						if(fsm[fsm_row][0] != ST){
							fsm[fsm_row][0]++;
						}
					}
					else{
						fsm_row = (int)(history[btb_index] & hist_mask);

						//flush routine
						if(fsm[fsm_row][0] < 2 || pred_dst != targetPc){
							ptr_table[btb_index].target = targetPc;
							stats->flush_num ++;
						}

						if(fsm[fsm_row][0] != ST){
							fsm[fsm_row][0]++;
						}	
					};
				}
				
				else{
					if(shared == USING_SHARE_LSB || shared == USING_SHARE_MID){
						int fsm_row = pc & shared_mask;	
						if(shared == USING_SHARE_LSB){
							fsm_row >>= 2;
						}
						else{
							fsm_row >>= 16;
						}

						fsm_row ^= (history[btb_index] & hist_mask);

						//flush routine
						if(fsm[fsm_row][0] > 1){
							stats->flush_num ++;
						}
						if(fsm[fsm_row][0] != SNT){
							fsm[fsm_row][0]--;
						}
					}

					else{
						fsm_row = (int)(history[btb_index] & hist_mask);

						//flush routine
						if(fsm[fsm_row][0] > 1){
							stats->flush_num ++;
						}

						if(fsm[fsm_row][0] != SNT){
							fsm[fsm_row][0]--;
						}	
					}
				}
				break;

			//global hist & gloal fsm
			case GH_GFSM:
				if(taken){
					if(shared == USING_SHARE_LSB || shared == USING_SHARE_MID){
						int fsm_row = pc & shared_mask;	
						if(shared == USING_SHARE_LSB){
							fsm_row >>= 2;
						}
						else{
							fsm_row >>= 16;
						}

						fsm_row ^= (history[0] & hist_mask);

						//flush routine
						if(fsm[fsm_row][0]  < 2 || pred_dst != targetPc){
							stats->flush_num ++;
						}	

						if(fsm[fsm_row][0] != ST){
							fsm[fsm_row][0]++;
						}
					}
					else{
						fsm_row = (int)(history[0] & hist_mask);

						//flush routine
						if(fsm[fsm_row][0]  < 2 || pred_dst != targetPc){
							stats->flush_num ++;
						}

						if(fsm[fsm_row][0] != ST){
							fsm[fsm_row][0]++;
						}	
					}
				}
				else{
					if(shared == USING_SHARE_LSB || shared == USING_SHARE_MID){
						int fsm_row = pc & shared_mask;	
						if(shared == USING_SHARE_LSB){
							fsm_row >>= 2;
						}
						else{
							fsm_row >>= 16;
						}

						fsm_row ^= (history[0] & hist_mask);

						//flush routine
						if(fsm[fsm_row][0] > 1){
							stats->flush_num ++;
						}

						if(fsm[fsm_row][0] != SNT){
							fsm[fsm_row][0]--;
						}
					}
					else{
						fsm_row = (int)(history[0] & hist_mask);

						//flush routine
						if(fsm[fsm_row][0] > 1){
							stats->flush_num ++;
						}

						if(fsm[fsm_row][0] != SNT){
							fsm[fsm_row][0]--;
						}	
					}
				}
				break;
		}//finish updating fsm and target

		//updating target
		ptr_table[btb_index].target = targetPc;

		//updating history vector
		if(history_fsm_state > 1){
			history[0] <<= 1;
			if(taken){
				history[0]++;
			}
			history[0] = history[0] & hist_mask;
		}
		else{
			history[btb_index] <<= 1;
			if(taken){
				history[btb_index]++;
			}
			history[btb_index] = history[btb_index] & hist_mask;
		}
		//finish updating history vector
	}
	
	//cuurent branch doesnt exist in Btb
	else{

		//when the branch wasnt found in Btb, we predicted not taken
		if(taken){
			stats->flush_num++;
		}

		//insert brnach to the table
		ptr_table[btb_index].tag = (pc & tag_mask) >> (2 + num_of_bits);
		ptr_table[btb_index].target = targetPc;

		//update history vector and fsm table according to history_fsm_state
		int fsm_size = (int)pow(2, historySize);
		switch(history_fsm_state){

			//local hist & local fsm
			case LH_LFSM:

				//update fsm
				fsmInitiate(fsm, fsm_size, btb_index, fsmState);
				if(taken && fsm[0][btb_index] != ST){
					fsm[0][btb_index]++;
				}
				else if(!taken && fsm[0][btb_index] != SNT){
					fsm[0][btb_index]--;
				}

				//update history
				history[btb_index] = 0;
				if(taken){
					history[btb_index]++;
				}
				history[btb_index] = history[btb_index] & hist_mask;				
				break;

			//global hist & local fsm
			case GH_LFSM:
				fsm_row = int(history[0] & hist_mask);

				//update fsm
				fsmInitiate(fsm, fsm_size, btb_index, fsmState);
				if(taken && fsm[fsm_row][btb_index] != ST){
					fsm[fsm_row][btb_index]++;
				}
				else if(!taken && fsm[fsm_row][btb_index] != SNT){
					fsm[fsm_row][btb_index]--;
				}

				//update history
				history[0] <<= 1;
				if(taken){
					history[0]++;
				}
				history[0] = history[0] & hist_mask;
				break;
			//local hist & global fsm
			case LH_GFSM:
			
				//flush history vector
				history[btb_index] = 0;

				//calculate fsm_row
				if(shared == USING_SHARE_LSB || shared == USING_SHARE_MID){
					fsm_row = pc & shared_mask;	
					if(shared == USING_SHARE_LSB){
						fsm_row >>= 2;
					}
					else{
						fsm_row >>= 16;
					}

					fsm_row ^= (history[btb_index] & hist_mask);
				}
				else{
					fsm_row = int(history[btb_index] & hist_mask);
				}

				//update fsm
				if(taken && fsm[fsm_row][0] != ST){
					fsm[fsm_row][0]++;
				}
				else if(!taken && fsm[fsm_row][0] != SNT){
					fsm[fsm_row][0]--;
				}

				//update history vector
				if(taken){
					history[btb_index]++;
				}
				history[btb_index] = history[btb_index] & hist_mask;
				break;

			//global hist & global fsm
			case GH_GFSM:

				//calculate fsm_row
				if(shared == USING_SHARE_LSB || shared == USING_SHARE_MID){
					fsm_row = pc & shared_mask;	
					if(shared == USING_SHARE_LSB){
						fsm_row >>= 2;
					}
					else{
						fsm_row >>= 16;
					}

					fsm_row ^= (history[0] & hist_mask);
				}
				else{
					fsm_row = int(history[0] & hist_mask);
				}

				//update fsm
				if(taken && fsm[fsm_row][0] != ST){
					fsm[fsm_row][0]++;
				}
				else if(!taken && fsm[fsm_row][0] != SNT){
					fsm[fsm_row][0]--;
				}

				//update history
				history[0] <<= 1;
				if(taken){
					history[0]++;
				}
				history[0] = history[0] & hist_mask;
				break;
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
	tag >>= (2 + num_of_bits);

	//current branch doesnt exists in Btb -> predict not taken
	if(ptr_table[btb_index].tag != tag){
		*dst = pc + 4;
		return false;
	}

	int history_column = isGlobalHist ? 0 : btb_index; 
	int fsm_column = isGlobalTable ? 0 : btb_index;
	int prediction;

	if(isGlobalTable && shared != NOT_USING_SHARE){
		unsigned fsm_index = pc & shared_mask;	
		if(shared == USING_SHARE_LSB){
			fsm_index >>= 2;
		}
		else{
			fsm_index >>= 16;
		}

		fsm_index = fsm_index ^ (history[history_column] & hist_mask);
		prediction = fsm[fsm_index][fsm_column];
	}
	else{
		prediction = fsm[(int)(history[history_column] & hist_mask)][fsm_column];	
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
	stats->size = btbSize*(tagSize + ADDRESS_LENGTH + VALID_BIT);

	//memory size from history vector
	if(isGlobalHist) {
		stats->size += historySize;
	}
	else{
		stats->size += btbSize*historySize;
	}

	//memory size from fsm
	if(isGlobalTable){
		stats->size += (int)pow(2, historySize)*2;
	}
	else{
		stats->size += btbSize*(int)pow(2, historySize)*2;
	}

	curStats->br_num = this->stats->br_num;
	curStats->flush_num = this->stats->flush_num;
	curStats->size = this->stats->size;
	return;
}

int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int shared){

	btb = new Btb(btbSize, historySize, tagSize, fsmState,
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
	delete btb;
	return;
}

