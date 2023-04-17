/* 046267 Computer Architecture - Winter 20/21 - HW #1                  */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"

//4 FSM states
#define SNT 0x00
#define WNT 0x01
#define WT 0x10
#define ST 0x11

using namespace std;

void fsmInitiate(char fsm[], int fsm_size){
	for(int i=0; i<fsm_size; i++){
		fsm[i] = SNT;
	}
}

Cell::Cell():
	tag(0),
	target(0),
	history(0){};

Cell::Cell(const Cell &cell): 
	tag(cell.tag),
	target(cell.target),
	history(cell.history){};

Btb::Btb(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
		 bool isGlobalHist, bool isGlobalTable, int Shared){
			this->btbSize = btbSize;
			this->historySize = historySize;
			this->tagSize = tagSize;
			this->fsmState = fsmState;
			this->isGlobalHist = isGlobalHist;
			this->isGlobalTable = isGlobalTable;
			this->Shared = Shared;
			this->ptr_table = new Cell[btbSize];
			this->fsm = new char[(int)pow(2, historySize)];
			fsmInitiate(this->fsm, (int)pow(2, historySize));

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

