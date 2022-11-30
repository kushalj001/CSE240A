//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "NAME";
const char *studentID   = "PID";
const char *email       = "EMAIL";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//
// gshare
uint32_t ghistoryMask;
uint32_t pcMask;
uint8_t *gshareTable;
uint32_t globalHistory;
// tournament
uint8_t *globalHistoryTable;
// indexed by global history bits and records the common case of global branches
uint8_t *localHistoryTable;
// indexed by local history bits which are calculated using pc and local branch table. Records
// common case of the branch under consideration
uint8_t *choiceTable;
// used to choose if the global prediction is correct or local. If upon indexing with global history
// bits, the prediction is taken (strong or weak), use the pred from the global table, else from local
uint32_t *localBranchTable;
// records the local history of branches, indexed by pc bits
uint32_t lhistoryMask;
// custom
int theta;
int tablePerceptron;
int nWeights;
int upperLimit=127;
int lowerLimit=-128;
int **weightTable;
int *biasTable;
//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //
  globalHistory = 0;
  
  switch(bpType){
    case GSHARE:
      
      // ghistoryBits = 8; 
      // keeps track of global history; 01101101 where 0 means a branch was not taken and vice-versa
      // pcIndexBits = 8;
      // PC bits used to access the BHT for a particular branch; same as ghistoryBits for gshare

      ghistoryMask = (1 << ghistoryBits) - 1;
      // 2^ghistoryBits - 1. Works as a modulo operator to make sure that any result lies in the
      // range [0, 2^ghistoryBits-1]. This directly translates to the number of entries in the 
      // table. For instance with ghistoryBits = 8, there will be 256 entries indexed from 0 to 255.
      // As such to index this table, we need to ensure that the index lies in the same range. This
      // can achieved by &ing the result with ghistoryMask.
      int numEntries = 1 << ghistoryBits;
      gshareTable = (uint8_t*)malloc(sizeof(uint8_t) * numEntries);
      // BHT of size 4 bytes * number of entries.
      // This table records the 2-bit saturating counters of various branches.
      for(int i=0;i<numEntries;i++) {
          gshareTable[i] = 1;
          // initialize all the 2 bit counters to 1 which is weakly not taken.
      }
      
    case TOURNAMENT:
      //define mask, size of table and then table; init table
      {
        int globalTableSize;
        ghistoryMask = (1 << ghistoryBits) - 1;
        globalTableSize = 1 << ghistoryBits;
        globalHistoryTable = (uint8_t*) malloc(sizeof(uint8_t) * globalTableSize);
        for(int i=0;i<globalTableSize;i++){
          globalHistoryTable[i] = 1;
        }

        int localTableSize;
        lhistoryMask = (1 << lhistoryBits) - 1;
        localTableSize = 1 << lhistoryBits;
        localHistoryTable = (uint8_t*)malloc(sizeof(uint8_t) * localTableSize);
        for(int i=0;i<localTableSize;i++){
          localHistoryTable[i] = 1;
        }

        int localBranchTableSize;
        pcMask = (1 << pcIndexBits) - 1;
        localBranchTableSize = 1 << pcIndexBits;
        localBranchTable = (uint32_t*)malloc(sizeof(uint32_t) * localBranchTableSize);
        for(int i=0;i<localBranchTableSize;i++){
          localBranchTable[i] = 0;
        }

        int choiceTableSize;
        choiceTableSize = 1 << ghistoryBits;
        choiceTable = (uint8_t*)malloc(sizeof(uint8_t) * choiceTableSize);
        for(int i=0;i<choiceTableSize;i++){
          choiceTable[i] = 2;
          // weakly taken for global pred
        }

      }
      case CUSTOM:
      {
			weightTable = (int**) malloc(sizeof(int*) * tablePerceptron);
			for(int i=0; i<tablePerceptron; i++)
				weightTable[i] = (int*) calloc(nWeights, sizeof(int));
			biasTable = (int*) calloc(tablePerceptron, sizeof(int));
			break;
      }
    default:
      break;
  }
  

}

int perceptron_sum(uint32_t pc)
{
	int wIndex = pc & (tablePerceptron - 1);
	int sum = biasTable[wIndex];
	for(int i=0; i<nWeights; i++)
		sum = ((globalHistory>>i)&1) ? sum + weightTable[wIndex][i] : sum - weightTable[wIndex][i];
	return sum;
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //
  
  // Make a prediction based on the bpType
  switch (bpType) {

    case STATIC:
      return TAKEN;
    case GSHARE:
      {
        uint8_t pred;
        uint32_t ghistBits;
        uint32_t pcBits;
        uint32_t index;

        ghistBits = globalHistory & ghistoryMask;
        pcBits = pc & ghistoryMask;
        index = ghistBits ^ pcBits;
        pred = gshareTable[index];
        if(pred > 1){
          return TAKEN;
        }
        else {
          return NOTTAKEN;
        }
      }
    case TOURNAMENT:
      
      {
        uint8_t pred;
        uint32_t ghistBits;
        uint32_t pcBits;
        uint32_t lhistBits;
        uint32_t choiceTablePred;
        ghistBits = globalHistory & ghistoryMask;
        choiceTablePred = choiceTable[ghistBits];
        if(choiceTablePred > 1){
          pred = globalHistoryTable[ghistBits];
        }
        else{
          pcBits = pc & pcMask;
          lhistBits = localBranchTable[pcBits] & lhistoryMask;
          pred = localHistoryTable[lhistBits];
        }

        if(pred > 1){
          return TAKEN;
        }
        else{
          return NOTTAKEN;
        }
      }
    case CUSTOM:
      {
        int sum = perceptron_sum(pc);
        if(sum > 0)
          return TAKEN;
        else
          return NOTTAKEN;
      }    
    default:
      break;
    
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //
  

  switch(bpType){
    case GSHARE:
    {
      uint32_t ghistBits;
      uint32_t pcBits;
      uint32_t index;
      ghistBits = globalHistory & ghistoryMask;
      pcBits = pc & ghistoryMask;
      index = ghistBits ^ pcBits;
      // recalculate index to update the BHT

      if(outcome == TAKEN && gshareTable[index]<3){
        gshareTable[index]++;
      } else {
        if(outcome == NOTTAKEN && gshareTable[index]>0){
          gshareTable[index]--;
        }
      }
      // update saturating counters based on the true outcome and the current state of the counter
      globalHistory = globalHistory << 1 | outcome;
      // append new outcome to the history
      return;
    }
    case TOURNAMENT:
      
      {
        uint32_t ghistBits;
        uint32_t pcBits;
        uint32_t lhistBits;
        uint8_t globalTablePred;
        uint8_t localTablePred;
        uint8_t choiceTablePred;
        ghistBits = globalHistory & ghistoryMask;
        choiceTablePred = choiceTable[ghistBits];
        globalTablePred = globalHistoryTable[ghistBits];


        if(globalTablePred > 1){
          globalTablePred = TAKEN;
        } else{
          globalTablePred = NOTTAKEN;
        }

        pcBits = pc & pcMask;
        lhistBits = lhistoryMask & localBranchTable[pcBits];
        localTablePred = localHistoryTable[lhistBits];

        if(localTablePred > 1){
          localTablePred = TAKEN;
        } else{
          localTablePred = NOTTAKEN;
        }

        if(outcome==globalTablePred && outcome!=localTablePred && choiceTablePred!=3){
          choiceTable[ghistBits]++;
        } else{
          if(outcome==localTablePred && outcome!=globalTablePred && choiceTablePred!=0){
            choiceTable[ghistBits]--;
          }
        }

        if(outcome==TAKEN){
          if(globalHistoryTable[ghistBits]!=3){
            globalHistoryTable[ghistBits]++;
          }
          if(localHistoryTable[lhistBits]!=3){
            localHistoryTable[lhistBits]++;
          }
        }else{
          if(globalHistoryTable[ghistBits]!=0){
            globalHistoryTable[ghistBits]--;
          }
          if(localHistoryTable[lhistBits]!=0){
            localHistoryTable[lhistBits]--;
          }
        }

        globalHistory = globalHistory << 1 | outcome;
        globalHistory = globalHistory & ghistoryMask;
        localBranchTable[pcBits] = localBranchTable[pcBits] << 1 | outcome & lhistoryMask;
        return;
      }
    case CUSTOM:
      {
        int sum = perceptron_sum(pc);
        int wIndex = pc & (tablePerceptron - 1);
        int perceptronOutcome = sum>=0;
        //Train predictor only if prediction != outcome or perceptron sum is less than threshold 
        if (abs(sum) <= theta || perceptronOutcome != outcome) 
        {
          //Update bias table entries
          biasTable[wIndex] += (outcome==TAKEN) ? 1 : -1; 

          if(biasTable[wIndex] > upperLimit){
            biasTable[wIndex]--;
          }
          else if(biasTable[wIndex] < lowerLimit) {
            biasTable[wIndex]++;}
            
            //Update perceptron table entries 
            for (int i=0; i<nWeights; i++) 
            {
              weightTable[wIndex][i] += (((globalHistory>>i) & 1)==outcome) ? 1 : -1;
              if(weightTable[wIndex][i] > upperLimit){
                weightTable[wIndex][i]--;
              }
              else if(weightTable[wIndex][i] < lowerLimit) {
                weightTable[wIndex][i]++;
                }
          }}
          globalHistory = (globalHistory<<1 | outcome) & ((1<<nWeights) - 1);
      }
    default:
      break;

  }
  
}
