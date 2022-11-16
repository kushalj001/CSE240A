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
int ghistoryMask;
int pcMask;
int *gshareTable;

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
  switch(bpType){
    case GSHARE:
      // ghistoryBits = 8; 
      // keeps track of global history; 01101101 where 0 means a branch was not taken and vice-versa
      // pcIndexBits = 8;
      // PC bits used to access the BHT for a particular branch; same as ghistoryBits for gshare

      ghistoryMask = 1 << ghistoryBits - 1;
      // 2^ghistoryBits - 1. Works as a modulo operator to make sure that any result lies in the
      // range [0, 2^ghistoryBits-1]. This directly translates to the number of entries in the 
      // table. For instance with ghistoryBits = 8, there will be 256 entries indexed from 0 to 255.
      // As such to index this table, we need to ensure that the index lies in the same range. This
      // can achieved by &ing the result with ghistoryMask.
      pcMask = ghistoryMask;
      int numEntries = 1 << ghistoryBits;
      gshareTable = (int*)malloc(sizeof(int) * numEntries);
      // BHT of size 4 bytes * number of entries.
      // This table records the 2-bit saturating counters of various branches.
      for(int i=0;i<numEntries;i++) {
          gshareTable[i] = 1;
          // initialize all the 2 bit counters to 1 which is weakly not taken.
      }

  }
  

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

    // for gshare
    int pred;
    int ghistBits;
    int pcBits;
    int index;

    case STATIC:
      return TAKEN;
    case GSHARE:
      ghistBits = ghistoryBits & ghistoryMask;
      pcBits = pc & pcMask;
      index = ghistBits ^ pcBits;
      pred = gshareTable[index];
      if(pred > 1){
        return TAKEN;
      }
      else {
        return NOTTAKEN;
      }

    case TOURNAMENT:
    case CUSTOM:
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
      int ghistBits;
      int pcBits;
      int index;
      ghistBits = ghistoryBits & ghistoryMask;
      pcBits = pc & pcMask;
      index = ghistBits ^ pcBits;
      // recalculate index to update the BHT

      if(outcome == TAKEN && gshareTable[index]<3){
        gshareTable[index] += 1;
      } else {
        if(outcome == NOTTAKEN && gshareTable[index]>0){
          gshareTable[index] -= 1;
        }
      }
      // update saturating counters based on the true outcome and the current state of the counter
      ghistoryBits = ghistoryBits << 1 | outcome; 
      // append new outcome to the history
      return;
  }
  
}
