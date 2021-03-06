/*Michae Mullings
  CDA3101
  March 25, 2016
  Project 2 - Pipelining*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUMMEMORY 16 /* Maximum number of data words in memory */
#define NUMREGS 8    /* Number of registers */

/* Opcode values for instructions */
#define R 0
#define LW 35
#define SW 43
#define BNE 4
#define HALT 63

/* Funct values for R-type instructions */
#define ADD 32
#define SUB 34

/* Branch Prediction Buffer Values */
#define STRONGLYTAKEN 3
#define WEAKLYTAKEN 2
#define WEAKLYNOTTAKEN 1
#define STRONGLYNOTTAKEN 0

typedef struct IFIDStruct {
  unsigned int instr;              /* Integer representation of instruction */
  int PCPlus4;                     /* PC + 4 */
} IFIDType;

typedef struct IDEXStruct {
  unsigned int instr;              /* Integer representation of instruction */
  int PCPlus4;                     /* PC + 4 */
  int readData1;                   /* Contents of rs register */
  int readData2;                   /* Contents of rt register */
  int immed;                       /* Immediate field */
  int rsReg;                       /* Number of rs register */
  int rtReg;                       /* Number of rt register */
  int rdReg;                       /* Number of rd register */
  int branchTarget;                /* Branch target, obtained from immediate field */
} IDEXType;

typedef struct EXMEMStruct {
  unsigned int instr;              /* Integer representation of instruction */
  int aluResult;                   /* Result of ALU operation */
  int writeDataReg;                /* Contents of the rt register, used for store word */
  int writeReg;                    /* The destination register */
} EXMEMType;

typedef struct MEMWBStruct {
  unsigned int instr;              /* Integer representation of instruction */
  int writeDataMem;                /* Data read from memory */
  int writeDataALU;                /* Result from ALU operation */
  int writeReg;                    /* The destination register */
} MEMWBType;

typedef struct stateStruct {
  int PC;                                 /* Program Counter */
  unsigned int instrMem[NUMMEMORY];       /* Instruction memory */
  int dataMem[NUMMEMORY];                 /* Data memory */
  int regFile[NUMREGS];                   /* Register file */
  IFIDType IFID;                          /* Current IFID pipeline register */
  IDEXType IDEX;                          /* Current IDEX pipeline register */
  EXMEMType EXMEM;                        /* Current EXMEM pipeline register */
  MEMWBType MEMWB;                        /* Current MEMWB pipeline register */
  int cycles;                             /* Number of cycles executed so far */
} stateType;


void run();
void printState(stateType*);
void initState(stateType*);
unsigned int instrToInt(char*, char*);
int get_opcode(unsigned int);
void printInstruction(unsigned int);

int main(){
    run();
    return(0); 
}

void run(){

  stateType state;           /* Contains the state of the entire pipeline before the cycle executes */ 
  stateType newState;        /* Contains the state of the entire pipeline after the cycle executes */
  initState(&state);         /* Initialize the state of the pipeline */
  
  int cyclestall = 0;
  int cyclebranch = 0;
  int mispredbrnch = 0;
  unsigned int bpb[200];
	
  int i;
  for (i = 0; i < 20; i++)
	bpb[i] = 1;  
 
    while (1) {

        printState(&state);

	/* If a halt instruction is entering its WB stage, then all of the legitimate */
	/* instruction have completed. Print the statistics and exit the program. */
        if (get_opcode(state.MEMWB.instr) == HALT) {
            printf("Total number of cycles executed: %d\n", state.cycles);
	    printf("Total number of stalls: %d\n", cyclestall);
	    printf("Total number of branches: %d\n", cyclebranch);
	    printf("Total number of mispredicted branches: %d\n", mispredbrnch);
            /* Remember to print the number of stalls, branches, and mispredictions! */
            exit(0);
        }

        newState = state;     /* Start by making newState a copy of the state before the cycle */
        newState.cycles++;

	/* Modify newState stage-by-stage below to reflect the state of the pipeline after the cycle has executed */

        /* --------------------- IF stage --------------------- */
	 newState.IFID.instr = state.instrMem[(state.PC)/4];
	 newState.PC = newState.PC+4;
	 newState.IFID.PCPlus4 = newState.PC;

        /* --------------------- ID stage --------------------- */     
	if(get_opcode(state.MEMWB.instr) == R && newState.MEMWB.instr != 0)
	{
	  newState.regFile[state.MEMWB.writeReg] = state.MEMWB.writeDataALU; 
	}
	else if (get_opcode(state.MEMWB.instr) == LW)
	{
	  newState.regFile[state.MEMWB.writeReg] = state.dataMem[(state.MEMWB.writeDataALU)/4];
	}	
	
	newState.IDEX.instr = state.IFID.instr;

	/*****************************Stalling**********************************************/
	if ( get_opcode(state.IDEX.instr) == LW && ( 
		( state.IDEX.rtReg == get_rs(state.IFID.instr) ) 
		|| ( state.IDEX.rtReg == get_rt(state.IFID.instr) )  ) 
										)
		{
		  newState.IFID.instr = state.IFID.instr; 
		  newState.IFID.PCPlus4 = state.IFID.PCPlus4;
		  newState.PC = state.PC;
		  newState.IDEX.instr = 0;
		  cyclestall+=1;
		}

	/***************************************************************************************/


	/******************************BRANCH PREDICTION********************************************/
	if (get_opcode(state.IFID.instr) == BNE)
	{
		if (bpb[(state.IFID.PCPlus4-4)/4] > 1)
		{
			newState.IFID.instr = 0;
			newState.IFID.PCPlus4 = 0; 
			newState.PC = get_immed(state.IFID.instr);
			cyclebranch++;
			cyclestall++;
		}
	}
	
	/*******************************************************************************************/

	if (newState.IDEX.instr != 0) 
	{	
		newState.IDEX.PCPlus4 = state.IFID.PCPlus4;
		newState.IDEX.immed = get_immed(state.IFID.instr);
		newState.IDEX.rsReg = get_rs(state.IFID.instr);
		newState.IDEX.rtReg = get_rt(state.IFID.instr);
		newState.IDEX.rdReg = get_rd(state.IFID.instr);
		newState.IDEX.branchTarget = newState.IDEX.immed; 
		newState.IDEX.readData1 = newState.regFile[newState.IDEX.rsReg]; 
		newState.IDEX.readData2 = newState.regFile[newState.IDEX.rtReg];
	}
	else
	{
		newState.IDEX.PCPlus4 = 0;
		newState.IDEX.immed = 0;
		newState.IDEX.rsReg = 0;
		newState.IDEX.rtReg = 0;
                newState.IDEX.rdReg = 0;
                newState.IDEX.branchTarget = 0; 
                newState.IDEX.readData1 = 0; 
                newState.IDEX.readData2 = 0;
	}
	

        /* --------------------- EX stage --------------------- */
	newState.EXMEM.instr = state.IDEX.instr;

	/************************************ Forwarding************************************/
	
	if (state.IDEX.rsReg == state.EXMEM.writeReg)
	{
		if (get_opcode(state.EXMEM.instr) == R && state.EXMEM.instr != 0 )
		{
			state.IDEX.readData1 = state.EXMEM.aluResult; 
		}
	}
	if ( (state.IDEX.rsReg == state.MEMWB.writeReg) && state.MEMWB.instr != 0 )
	{
		state.IDEX.readData1 = state.MEMWB.writeDataMem;
	}
	
	if (state.IDEX.rtReg == state.EXMEM.writeReg )
	{
		
		if (get_opcode(state.EXMEM.instr) == R && state.EXMEM.instr != 0 )
		{
			state.IDEX.readData2 = state.EXMEM.aluResult; 
		}
	}
	if ( (state.IDEX.rtReg == state.MEMWB.writeReg) && state.MEMWB.instr != 0 )
	{
		state.IDEX.readData2 = state.MEMWB.writeDataMem; 
	}
	
	/*************************Forwarding******************************************/

	if (get_opcode(newState.EXMEM.instr) == LW)
		{
			newState.EXMEM.aluResult = state.IDEX.readData1+state.IDEX.immed;
			newState.EXMEM.writeDataReg = state.IDEX.readData2;
			newState.EXMEM.writeReg = state.IDEX.rtReg;
		}
	else if (get_opcode(newState.EXMEM.instr) == SW)
		{
		  newState.EXMEM.aluResult = state.IDEX.readData1+state.IDEX.immed;
		  newState.EXMEM.writeDataReg = state.IDEX.readData2;
		  newState.EXMEM.writeReg = state.IDEX.rtReg;
		}
	else if (get_opcode(newState.EXMEM.instr) == BNE)
		{
		  newState.EXMEM.aluResult = state.IDEX.readData1-state.IDEX.readData2;
		  newState.EXMEM.writeDataReg = state.IDEX.readData2;
		  newState.EXMEM.writeReg = state.IDEX.rtReg;
		}
	else if (get_opcode(newState.EXMEM.instr) == R)
		{
			if(get_funct(newState.EXMEM.instr) == ADD)
			{
				newState.EXMEM.aluResult = state.IDEX.readData1+state.IDEX.readData2;
				newState.EXMEM.writeDataReg = state.IDEX.readData2;
				newState.EXMEM.writeReg = state.IDEX.rdReg;
			}
			else if (get_funct(newState.EXMEM.instr) == SUB)
			{
				newState.EXMEM.aluResult = state.IDEX.readData1-state.IDEX.readData2;
				newState.EXMEM.writeDataReg = state.IDEX.readData2;
				newState.EXMEM.writeReg = state.IDEX.rdReg;
			}

		}
	else if (get_opcode(newState.EXMEM.instr) == HALT)
		{
			/*newState.EXMEM.aluResult = state.IDEX.readData1+state.IDEX.readData2;
			newState.EXMEM.writeDataReg = state.IDEX.readData2;*/
			newState.EXMEM.writeReg = state.IDEX.rdReg;
		}

/**************************************************BRANCH PREDICTION********************************************/
	if (get_opcode(state.IDEX.instr) == BNE)
	{
		// Predicted taken and decision is taken
		if ( bpb[(state.IDEX.PCPlus4-4)/4] > 1 && newState.EXMEM.aluResult != 0 )
		{
			 bpb[(state.IDEX.PCPlus4-4)/4]=STRONGLYTAKEN;
		}
		else if ( bpb[(state.IDEX.PCPlus4-4)/4] <= 1 && newState.EXMEM.aluResult == 0 )
		{	// predicted not taken and was not taken
			bpb[(state.IDEX.PCPlus4-4)/4]=STRONGLYNOTTAKEN;
		}
		else if ( bpb[(state.IDEX.PCPlus4-4)/4] > 1 && newState.EXMEM.aluResult == 0 )
		{	// predicted taken, but was not taken
			// NOOP FOR IFID
			newState.IFID.instr = 0;
			newState.IFID.PCPlus4 = 0;
			
			// NOOP FOR IDEX
			newState.IDEX.instr = 0;
			newState.IDEX.PCPlus4 = 0;
                	newState.IDEX.immed = 0;
                	newState.IDEX.rsReg = 0;
                	newState.IDEX.rtReg = 0;
                	newState.IDEX.rdReg = 0;
                	newState.IDEX.branchTarget = 0; 
                	newState.IDEX.readData1 = 0; 
        	        newState.IDEX.readData2 = 0;
			
			
			bpb[(state.IDEX.PCPlus4-4)/4] -= 1;

			newState.PC = state.IDEX.PCPlus4;
			
			cyclestall+=1;
			mispredbrnch++;

		}
		else if ( bpb[(state.IDEX.PCPlus4-4)/4] <= 1 && newState.EXMEM.aluResult != 0 )
		{
			// NOOP FOR IFID
                        newState.IFID.instr = 0;
                        newState.IFID.PCPlus4 = 0;
                        
                        // NOOP FOR IDEX
                        newState.IDEX.instr = 0;
                        newState.IDEX.PCPlus4 = 0;
                        newState.IDEX.immed = 0;
                        newState.IDEX.rsReg = 0;
                        newState.IDEX.rtReg = 0;
                        newState.IDEX.rdReg = 0;
                        newState.IDEX.branchTarget = 0; 
                        newState.IDEX.readData1 = 0; 
                        newState.IDEX.readData2 = 0;
			
			bpb[(state.IDEX.PCPlus4-4)/4] += 1;

			newState.PC = state.IDEX.branchTarget;
			cyclebranch+=1;
			cyclestall+=2;
			mispredbrnch+=1;
		}
	}
/*********************************************************************************************************************************/
		  
        /* --------------------- MEM stage --------------------- */
	newState.MEMWB.instr = state.EXMEM.instr;
	newState.MEMWB.writeDataALU = state.EXMEM.aluResult;	
	if(get_opcode(newState.MEMWB.instr) == LW)
	{
		newState.MEMWB.writeDataMem = state.dataMem[(newState.MEMWB.writeDataALU)/4]; 
	}
	if(get_opcode(newState.MEMWB.instr) == SW)
	{
		newState.dataMem[(newState.MEMWB.writeDataALU)/4] = newState.regFile[state.EXMEM.writeReg];
	}
	newState.MEMWB.writeReg = state.EXMEM.writeReg; 
       

	 /* --------------------- WB stage --------------------- */


        state = newState;    /* The newState now becomes the old state before we execute the next cycle */
    }
}


/******************************************************************/
/* The initState function accepts a pointer to the current        */ 
/* state as an argument, initializing the state to pre-execution  */
/* state. In particular, all registers are zero'd out. All        */
/* instructions in the pipeline are NOOPS. Data and instruction   */
/* memory are initialized with the contents of the assembly       */
/* input file.                                                    */
/*****************************************************************/
void initState(stateType *statePtr)
{
    unsigned int dec_inst;
    int data_index = 0;
    int inst_index = 0;
    char line[130];
    char instr[6];
    char args[130];
    char* arg; 

    statePtr->PC = 0;
    statePtr->cycles = 0;

    /* Zero out data, instructions, and registers */
    memset(statePtr->dataMem, 0, 4*NUMMEMORY);
    memset(statePtr->instrMem, 0, 4*NUMMEMORY);
    memset(statePtr->regFile, 0, 4*NUMREGS);

    /* Parse assembly file and initialize data/instruction memory */
    while(fgets(line, 130, stdin)){
        if(sscanf(line, "\t.%s %s", instr, args) == 2){
            arg = strtok(args, ",");
            while(arg != NULL){
                statePtr->dataMem[data_index] = atoi(arg);
                data_index += 1;
                arg = strtok(NULL, ","); 
            }  
        }
        else if(sscanf(line, "\t%s %s", instr, args) == 2){
            dec_inst = instrToInt(instr, args);
            statePtr->instrMem[inst_index] = dec_inst;
            inst_index += 1;
        }
    } 

    /* Zero-out all registers in pipeline to start */
    statePtr->IFID.instr = 0;
    statePtr->IFID.PCPlus4 = 0;
    statePtr->IDEX.instr = 0;
    statePtr->IDEX.PCPlus4 = 0;
    statePtr->IDEX.branchTarget = 0;
    statePtr->IDEX.readData1 = 0;
    statePtr->IDEX.readData2 = 0;
    statePtr->IDEX.immed = 0;
    statePtr->IDEX.rsReg = 0;
    statePtr->IDEX.rtReg = 0;
    statePtr->IDEX.rdReg = 0;
 
    statePtr->EXMEM.instr = 0;
    statePtr->EXMEM.aluResult = 0;
    statePtr->EXMEM.writeDataReg = 0;
    statePtr->EXMEM.writeReg = 0;

    statePtr->MEMWB.instr = 0;
    statePtr->MEMWB.writeDataMem = 0;
    statePtr->MEMWB.writeDataALU = 0;
    statePtr->MEMWB.writeReg = 0;
 }


 /***************************************************************************************/
 /*              You do not need to modify the functions below.                         */
 /*                They are provided for your convenience.                              */
 /***************************************************************************************/


/*************************************************************/
/* The printState function accepts a pointer to a state as   */
/* an argument and prints the formatted contents of          */
/* pipeline register.                                        */
/* You should not modify this function.                      */
/*************************************************************/
void printState(stateType *statePtr)
{
    int i;
    printf("\n********************\nState at the beginning of cycle %d:\n", statePtr->cycles+1);
    printf("\tPC = %d\n", statePtr->PC);
    printf("\tData Memory:\n");
    for (i=0; i<(NUMMEMORY/2); i++) {
        printf("\t\tdataMem[%d] = %d\t\tdataMem[%d] = %d\n", 
            i, statePtr->dataMem[i], i+(NUMMEMORY/2), statePtr->dataMem[i+(NUMMEMORY/2)]);
    }
    printf("\tRegisters:\n");
    for (i=0; i<(NUMREGS/2); i++) {
        printf("\t\tregFile[%d] = %d\t\tregFile[%d] = %d\n", 
            i, statePtr->regFile[i], i+(NUMREGS/2), statePtr->regFile[i+(NUMREGS/2)]);
    }
    printf("\tIF/ID:\n");
    printf("\t\tInstruction: ");
    printInstruction(statePtr->IFID.instr);
    printf("\t\tPCPlus4: %d\n", statePtr->IFID.PCPlus4);
    printf("\tID/EX:\n");
    printf("\t\tInstruction: ");
    printInstruction(statePtr->IDEX.instr);
    printf("\t\tPCPlus4: %d\n", statePtr->IDEX.PCPlus4);
    printf("\t\tbranchTarget: %d\n", statePtr->IDEX.branchTarget);
    printf("\t\treadData1: %d\n", statePtr->IDEX.readData1);
    printf("\t\treadData2: %d\n", statePtr->IDEX.readData2);
    printf("\t\timmed: %d\n", statePtr->IDEX.immed);
    printf("\t\trs: %d\n", statePtr->IDEX.rsReg);
    printf("\t\trt: %d\n", statePtr->IDEX.rtReg);
    printf("\t\trd: %d\n", statePtr->IDEX.rdReg);
    printf("\tEX/MEM:\n");
    printf("\t\tInstruction: ");
    printInstruction(statePtr->EXMEM.instr);
    printf("\t\taluResult: %d\n", statePtr->EXMEM.aluResult);
    printf("\t\twriteDataReg: %d\n", statePtr->EXMEM.writeDataReg);
    printf("\t\twriteReg:%d\n", statePtr->EXMEM.writeReg);
    printf("\tMEM/WB:\n");
    printf("\t\tInstruction: ");
    printInstruction(statePtr->MEMWB.instr);
    printf("\t\twriteDataMem: %d\n", statePtr->MEMWB.writeDataMem);
    printf("\t\twriteDataALU: %d\n", statePtr->MEMWB.writeDataALU);
    printf("\t\twriteReg: %d\n", statePtr->MEMWB.writeReg);
}

/*************************************************************/
/*  The instrToInt function converts an instruction from the */
/*  assembly file into an unsigned integer representation.   */
/*  For example, consider the add $0,$1,$2 instruction.      */
/*  In binary, this instruction is:                          */
/*   000000 00001 00010 00000 00000 100000                   */
/*  The unsigned representation in decimal is therefore:     */
/*   2228256                                                 */
/*************************************************************/
unsigned int instrToInt(char* inst, char* args){

    int opcode, rs, rt, rd, shamt, funct, immed;
    unsigned int dec_inst;
    
    if((strcmp(inst, "add") == 0) || (strcmp(inst, "sub") == 0)){
        opcode = 0;
        if(strcmp(inst, "add") == 0)
            funct = ADD;
        else
            funct = SUB; 
        shamt = 0; 
        rd = atoi(strtok(args, ",$"));
        rs = atoi(strtok(NULL, ",$"));
        rt = atoi(strtok(NULL, ",$"));
        dec_inst = (opcode << 26) + (rs << 21) + (rt << 16) + (rd << 11) + (shamt << 6) + funct;
    } else if((strcmp(inst, "lw") == 0) || (strcmp(inst, "sw") == 0)){
        if(strcmp(inst, "lw") == 0)
            opcode = LW;
        else
            opcode = SW;
        rt = atoi(strtok(args, ",$"));
        immed = atoi(strtok(NULL, ",("));
        rs = atoi(strtok(NULL, "($)"));
        dec_inst = (opcode << 26) + (rs << 21) + (rt << 16) + immed;
    } else if(strcmp(inst, "bne") == 0){
        opcode = 4;
        rs = atoi(strtok(args, ",$"));
        rt = atoi(strtok(NULL, ",$"));
        immed = atoi(strtok(NULL, ","));
        dec_inst = (opcode << 26) + (rs << 21) + (rt << 16) + immed;   
    } else if(strcmp(inst, "halt") == 0){
        opcode = 63; 
        dec_inst = (opcode << 26);
    } else if(strcmp(inst, "noop") == 0){
        dec_inst = 0;
    }
    return dec_inst;
}

int get_rs(unsigned int instruction){
    return( (instruction>>21) & 0x1F);
}

int get_rt(unsigned int instruction){
    return( (instruction>>16) & 0x1F);
}

int get_rd(unsigned int instruction){
    return( (instruction>>11) & 0x1F);
}

int get_funct(unsigned int instruction){
    return(instruction & 0x3F);
}

int get_immed(unsigned int instruction){
    return(instruction & 0xFFFF);
}

int get_opcode(unsigned int instruction){
    return(instruction>>26);
}

/*************************************************/
/*  The printInstruction decodes an unsigned     */
/*  integer representation of an instruction     */
/*  into its string representation and prints    */
/*  the result to stdout.                        */
/*************************************************/
void printInstruction(unsigned int instr)
{
    char opcodeString[10];
    if (instr == 0){
      printf("NOOP \n");
    } else if (get_opcode(instr) == R) {
        if(get_funct(instr)!=0){
            if(get_funct(instr) == ADD)
                strcpy(opcodeString, "add");
            else
                strcpy(opcodeString, "sub");
            printf("%s $%d,$%d,$%d\n", opcodeString, get_rd(instr), get_rs(instr), get_rt(instr));
        }
        else{
            printf("NOOP \n");
        }
    } else if (get_opcode(instr) == LW) {
        printf("%s $%d,%d($%d)\n", "lw", get_rt(instr), get_immed(instr), get_rs(instr));
    } else if (get_opcode(instr) == SW) {
        printf("%s $%d,%d($%d)\n", "sw", get_rt(instr), get_immed(instr), get_rs(instr));
    } else if (get_opcode(instr) == BNE) {
        printf("%s $%d,$%d,%d\n", "bne", get_rs(instr), get_rt(instr), get_immed(instr));
    } else if (get_opcode(instr) == HALT) {
        printf("%s\n", "halt");
    }
}

