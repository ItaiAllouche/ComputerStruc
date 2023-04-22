/* 046267 Computer Architecture - Winter 20/21 - HW #1 */
/* API for the predictor simulator                     */

#ifndef BP_API_H_
#define BP_API_H_

#include <cstdio>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

/* A structure to return information about the currect simulator state */
typedef struct {
	unsigned flush_num;           // Machine flushes
	unsigned br_num;      	      // Number of branch instructions
	unsigned size;		      // Theoretical allocated BTB and branch predictor size
} SIM_stats;

/*************************************************************************/
/* The following functions should be implemented in your bp.c (or .cpp) */
/*************************************************************************/

/*
 * BP_init - initialize the predictor
 * all input parameters are set (by the main) as declared in the trace file
 * return 0 on success, otherwise (init failure) return <0
 */
int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
bool isGlobalHist, bool isGlobalTable, int shared);

/*
 * BP_predict - returns the predictor's prediction (taken / not taken) and predicted target address
 * param[in] pc - the branch instruction address
 * param[out] dst - the target address (when prediction is not taken, dst = pc + 4)
 * return true when prediction is taken, otherwise (prediction is not taken) return false
 */
bool BP_predict(uint32_t pc, uint32_t *dst);

/*
 * BP_update - updates the predictor with actual decision (taken / not taken)
 * param[in] pc - the branch instruction address
 * param[in] targetPc - the branch instruction target address
 * param[in] taken - the actual decision, true if taken and false if not taken
 * param[in] pred_dst - the predicted target address
 */
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);

/*
 * BP_GetStats: Return the simulator stats using a pointer
 * curStats: The returned current simulator state (only after BP_update)
 */
void BP_GetStats(SIM_stats *curStats);


//Cell class for cell in BTB
class Cell{
public:
	int pc;
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
	char hist_mask;
	SIM_stats* stats;

public:
	Btb(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int shared);

	~Btb();

	bool predict(uint32_t pc, uint32_t *dst);

	void update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst);

	void getStats(SIM_stats *curStats);
};


#endif /* BP_API_H_ */
