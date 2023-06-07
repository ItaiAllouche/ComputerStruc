/* 046267 Computer Architecture - HW #3 */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"
#include "cstdio"

/*########## DEBUG MACROS ##########*/
// #define DEBUG_PRINT_NODES
// #define DEBUG_PRINT_FULL_NODES
// #define DEBUG_PRINT_DIRTY_REGS
/*##################################*/


/*############ MACROS ############*/
#define ERROR -1
#define SUCCESS 0
#define UNUSED -1
#define ENTRY -1
/*################################*/

/*############ CLASSES ###########*/
class Node{
    public:
        InstInfo inst;
        int latency;
        int dep_idx1;
        int dep_idx2;
        int depth;
        
        //constractor
        Node(void);
};

/**
 * @brief implemented with array, each command placed in the array
 *  by its command index 
 */
class DepGraph{
    public:
        Node* graph;
        unsigned int length;

        // contains index of last command which change the register
        int* dirty_regs;

        // constractor
        DepGraph(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts);

        // distrctor
        ~DepGraph();

        // given an instuction's index, return its depth (ints -> entry)
        int getInsDeps(unsigned int instIdx, int *src1DepInst, int *src2DepInst);

        // initite DirtyReg to -1
        void initDirtyRegs(void);

        // print dirtyReg - enabled by debug macro
        void printDirtyRegs(void);
};
/*################################*/

Node::Node(){
    this->latency = 0;
    this->depth = 0;
    this->dep_idx1 = UNUSED;
    this->dep_idx2 = UNUSED;
}

void DepGraph::initDirtyRegs(){
    for(int i = 0; i < MAX_OPS; i++){
        this->dirty_regs[i] = UNUSED;
    }
}

void DepGraph::printDirtyRegs(){
        printf("dirty regs:\n");
        for(int i = 0; i < MAX_OPS; i++){
            printf("|%d|",dirty_regs[i]);
        }
}

DepGraph::DepGraph(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts){
    this->graph = new Node[numOfInsts];
    this->length = numOfInsts;
    this->dirty_regs = new int[MAX_OPS];

    //initilize dirty registers array
    this->initDirtyRegs();

    for(unsigned int i = 0; i < numOfInsts; i++){
        graph[i].inst = progTrace[i];
        graph[i].latency = opsLatency[graph[i].inst.opcode];

        #ifdef DEBUG_PRINT_FULL_NODES
        printf("|idx %d|opcode %d|dest %d|src1 %d|src2 %d|latency %d|\n", i, graph[i].inst.opcode, graph[i].inst.dstIdx, 
                                                        graph[i].inst.src1Idx, graph[i].inst.src2Idx, graph[i].latency);
        #endif

        int dep1 = graph[i].latency;
        int dep2 = graph[i].latency;

        // update first dependencey
        graph[i].dep_idx1 = dirty_regs[graph[i].inst.src1Idx];
        if(graph[i].dep_idx1 != UNUSED){
            dep1 += graph[graph[i].dep_idx1].depth;
        }

        // update second dependencey
        graph[i].dep_idx2 = dirty_regs[graph[i].inst.src2Idx];
        if(graph[i].dep_idx2 != UNUSED){
            dep2 += graph[graph[i].dep_idx2].depth;
        }
        
        // update modifed register by current command's index
        dirty_regs[graph[i].inst.dstIdx] = i;

        // update depth from "entry node"
        graph[i].depth = dep1 > dep2 ? dep1 : dep2;
        
        #ifdef DEBUG_PRINT_NODES
        printf("|cmd %d|dep1 %d|dep2 %d|depth %d|latency %d|\n",i,graph[i].dep_idx1,graph[i].dep_idx2,graph[i].depth,graph[i].latency);
        #endif

        #ifdef DEBUG_PRINT_DIRTY_REGS
        printDirtyRegs();
        #endif
    }
    
}

DepGraph::~DepGraph(){
    delete[] this->graph;
    delete this->dirty_regs;
}

int DepGraph::getInsDeps(unsigned int instIdx, int *src1DepInst, int *src2DepInst){

    //index is invalid
    if(instIdx < 0 || instIdx > (length + 1)){
        return ERROR;
    }
    else{
        *src1DepInst = graph[instIdx].dep_idx1;
        *src2DepInst = graph[instIdx].dep_idx2;
    }
    return SUCCESS;
}

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    DepGraph* dep_graph = new DepGraph(opsLatency, progTrace, numOfInsts);
    return dep_graph;
}

void freeProgCtx(ProgCtx ctx) {
    DepGraph* dep_graph = (DepGraph*)ctx;
    delete dep_graph;
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    DepGraph* dep_graph = (DepGraph*)ctx;

    // invalid index
    if(theInst < 0 || theInst > (dep_graph->length - 1)){
        return ERROR;
    }
    return (dep_graph->graph[theInst].depth - dep_graph->graph[theInst].latency);
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    DepGraph* dep_graph = (DepGraph*)ctx;
    return dep_graph->getInsDeps(theInst, src1DepInst, src2DepInst);
}

int getProgDepth(ProgCtx ctx) {
    DepGraph* dep_graph = (DepGraph*)ctx;
    int max_depth = 0;
    int current_depth = 0;

    for(unsigned int i = 0; i < dep_graph->length; i++){
        current_depth = dep_graph->graph[i].depth;

        // update max depth if necessary
        if(current_depth > max_depth){
            max_depth = current_depth;
        } 
        
    }
    return max_depth;
}


