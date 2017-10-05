/**************************************************************/
/* CS/COE 1541
 Noah Perryman
 Patrick Lyons
 just compile with gcc -o superscalar superscalar.c
 and execute using
 /superscalar 'trace file' 'prediction_method' 'trace_vew_on'
 ***************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "CPU.h"

int checkPipeline(struct trace_item **item);
void insert_NOP(struct trace_item **item, int stage);
void shift(struct trace_item **item);
struct trace_item temp;
void hazard_switch(struct trace_item **item);
int check_data_hazard(struct trace_item **item);

int main(int argc, char **argv)
{
    struct trace_item *tr_entry;
    size_t size1, size2;
    char *trace_file_name;
    int prediction_method = 0; //branch prediction on or off. 0 for predict not taken, 1 for 1-bit prediction. Default value is 0
    int trace_view_on = 0; //Cycle output printed to screen on or off. 0 for off, 1 for on
    //struct trace_item *item;
    struct trace_item *tr_pipeline1;
    struct trace_item *tr_pipeline2;
    struct trace_item *instruction_buffer = malloc(sizeof(struct trace_item)*2);
    //struct trace_item *left_pipeline;
    
    /*unsigned char t_type = 0;
     unsigned char t_sReg_a= 0;
     unsigned char t_sReg_b= 0;
     unsigned char t_dReg= 0;
     unsigned int t_PC = 0;
     unsigned int t_Addr = 0;*/
    
    unsigned int cycle_number = 0;
    
    if (argc == 1) { //If the input does not include the filename there is nothing to trace (i.e. no input), so exit
        fprintf(stdout, "\nUSAGE: tv <trace_file> <switch - any character>\n");
        fprintf(stdout, "\n(switch) to turn on or off individual item view.\n\n");
        exit(0);
    }
    
    if (argc > 4) {
        fprintf(stdout, "\nToo many arguments innput\n\n");
        exit(0);
    }
    
    trace_file_name = argv[1]; //filename is the second argument, so take it
    if (argc == 3) {
        prediction_method = atoi(argv[2]); //prediction_method will be changed from defualt if it is included. It is the third argument
        if (prediction_method < 0 || prediction_method > 1) {
            fprintf(stdout, "\nInvalid input arguments\n\n"); //check if input is invalid
            exit(0);
        }
    }else if (argc == 4) {
        prediction_method = atoi(argv[2]); //prediction_method will be changed from defualt if it is included. It is the third argument
        trace_view_on = atoi(argv[3]) ; //trace_view_on will be changed from defualt if it is included. It is the fourth argument
        if (prediction_method < 0 || prediction_method > 1 || trace_view_on < 0 || trace_view_on > 1) {
            fprintf(stdout, "\nInvalid input arguments\n\n");
            exit(0);
        }
    }
    
    fprintf(stdout, "\n ** opening file %s\n", trace_file_name);
    
    trace_fd = fopen(trace_file_name, "rb"); //open file to be read
    
    if (!trace_fd) { //if file fopen() returns an error, exit
        fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
        exit(0);
    }
    
    trace_init(); //initialize the trace
    
    int k;
    for(k = 0; k < 5; k++) {
        insert_NOP(&tr_pipeline1, k);
        insert_NOP(&tr_pipeline2, k);
    }
    
    int *branch_predictor = malloc(sizeof(int)*64);
    for (k = 0; k < 64; k++) {
        branch_predictor[k] = 0;
    }
    
    while(1) {
        size1 = trace_get_item(&tr_entry);
        
        //printf("\n\nsize1: %zu\n\n",size1);
        if (!size1 && !checkPipeline(&tr_pipeline1) && !checkPipeline(&tr_pipeline2)) {       /* no more instructions (trace_items) to simulate */
            printf("+ Simulation terminates at cycle : %u\n", cycle_number);
            break;
        }else {
            shift(&tr_pipeline1);
            if (!size1) { //insert into pipeline
                tr_pipeline1[0].type = 0;
                instruction_buffer[0].type = 0;
            }else {
                tr_pipeline1[0] = *tr_entry;
                instruction_buffer[0] = *tr_entry;
                
            }
            
            if (check_data_hazard(&tr_pipeline1))
            {
                shift(&tr_pipeline1);
                insert_NOP(&tr_pipeline1, 5);
                hazard_switch(&tr_pipeline1);
                shift(&tr_pipeline1);
                hazard_switch(&tr_pipeline1);
            }
            
            //see if branch or jump first
            if (tr_pipeline1[2].type == ti_BRANCH || tr_pipeline1[2].type == ti_JTYPE || tr_pipeline1[2].type == ti_JRTYPE) {
                
                if (prediction_method == 0) {
                    
                    if (tr_pipeline1[1].PC != (tr_pipeline1[2].PC + 4)) {
                        //printf("\n\n%d\n%d\n\n",tr_pipeline1[1].type, tr_pipeline1[2].type);
                        tr_pipeline1[1].type = 0;//Squashed
                        tr_pipeline1[0].type = 0;//Squashed
                        //printf("\n\n%d\n%d\n\n",tr_pipeline1[1].type, tr_pipeline1[2].type);
                    }
                    
                }
                
                else {
                    
                    int index = tr_pipeline1[2].PC;
                    index = index >> 4;
                    index = index & 511;
                    
                    if(branch_predictor[index] == 0) {
                        
                        if (tr_pipeline1[1].PC != (tr_pipeline1[2].PC + 4)) {
                            tr_pipeline1[1].type = 0; //Squashed
                            tr_pipeline1[0].type = 0; //Squashed
                            branch_predictor[index] = 1;
                        }
                    }
                    
                    else {
                        
                        if (tr_pipeline1[2].Addr != tr_pipeline1[1].PC) {
                            tr_pipeline1[1].type = 0;//Squashed
                            tr_pipeline1[0].type = 0;//Squashed
                            branch_predictor[index] = 0;
                        }
                    }
                }
            }

            
        }
        
        size2 = trace_get_item(&tr_entry);
        
        if (!size2 && !checkPipeline(&tr_pipeline1) && !checkPipeline(&tr_pipeline2)) {
            printf("+ Simulation terminates at cycle : %u\n", cycle_number);
            break;
        }else{              /* parse the next instruction to simulate */
            //printf("\n\n%d\n\n",tr_entry1[0].type);
            cycle_number++;
            /*t_type = tr_entry->type;
            t_sReg_a = tr_entry->sReg_a;
            t_sReg_b = tr_entry->sReg_b;
            t_dReg = tr_entry->dReg;
            t_PC = tr_entry->PC;
            t_Addr = tr_entry->Addr;*/
            shift(&tr_pipeline2);
            if (!size2) { //insert into pipeline
                tr_pipeline2[0].type = 0;
                instruction_buffer[1].type = 0;
            }else {
                tr_pipeline2[0] = *tr_entry;
                instruction_buffer[1] = *tr_entry;
            }
            
            /*if ((instruction_buffer[1].type == ti_RTYPE || instruction_buffer[1].type == ti_BRANCH || instruction_buffer[1].type == ti_ITYPE || instruction_buffer[1].type == ti_JTYPE || instruction_buffer[1].type == ti_JRTYPE) && (instruction_buffer[0].type == ti_RTYPE || instruction_buffer[0].type == ti_BRANCH || instruction_buffer[0].type == ti_ITYPE || instruction_buffer[0].type == ti_JTYPE || instruction_buffer[0].type == ti_JRTYPE)) {
                
                tr_pipeline2[1].type = 0;
                
            }else if ((instruction_buffer[0].type == ti_LOAD || instruction_buffer[0].type == ti_STORE) && (instruction_buffer[1].type == ti_LOAD || instruction_buffer[1].type == ti_STORE)) {
                
                tr_pipeline1[1].type = 0;
                
            }*/
            
            if ((instruction_buffer[1].type == ti_RTYPE || instruction_buffer[1].type == ti_BRANCH || instruction_buffer[1].type == ti_ITYPE || instruction_buffer[1].type == ti_JTYPE || instruction_buffer[1].type == ti_JRTYPE) && (instruction_buffer[0].type == ti_LOAD || instruction_buffer[0].type == ti_STORE)) {
            
                struct trace_item *temporary = malloc(sizeof(struct trace_item));
                
                temporary[0] = instruction_buffer[0];
                instruction_buffer[0] = instruction_buffer[1];
                instruction_buffer[1] = temporary[0];
                
                free(temporary);
            }
            
            if ((instruction_buffer[0].type == ti_RTYPE || instruction_buffer[0].type == ti_BRANCH || instruction_buffer[0].type == ti_ITYPE || instruction_buffer[0].type == ti_JTYPE || instruction_buffer[0].type == ti_JRTYPE) && (instruction_buffer[1].type == ti_LOAD || instruction_buffer[1].type == ti_STORE)) {
             
                if(check_data_hazard(&instruction_buffer)) {
                    instruction_buffer[0].type = 0;
                    tr_pipeline1[0] = instruction_buffer[0];
                }
                
            }else if ((instruction_buffer[1].type == ti_RTYPE || instruction_buffer[1].type == ti_BRANCH || instruction_buffer[1].type == ti_ITYPE || instruction_buffer[1].type == ti_JTYPE || instruction_buffer[1].type == ti_JRTYPE) && (instruction_buffer[0].type == ti_LOAD || instruction_buffer[0].type == ti_STORE)) {
                
                if(check_data_hazard(&instruction_buffer)) {
                    instruction_buffer[1].type = 0;
                    tr_pipeline1[1] = instruction_buffer[0];
                }
                
            }
            
            if (check_data_hazard(&tr_pipeline2))
            {
                shift(&tr_pipeline2);
                insert_NOP(&tr_pipeline2, 5);
                hazard_switch(&tr_pipeline2);
                shift(&tr_pipeline2);
                hazard_switch(&tr_pipeline2);
            }
            
            //see if branch or jump first
            if (tr_pipeline2[2].type == ti_BRANCH || tr_pipeline2[2].type == ti_JTYPE || tr_pipeline2[2].type == ti_JRTYPE) {
                
                if (prediction_method == 0) {
                    
                    if (tr_pipeline2[1].PC != (tr_pipeline2[2].PC + 4)) {
                        //printf("\n\n%d\n%d\n\n",tr_pipeline2[1].type, tr_pipeline2[2].type);
                        tr_pipeline2[1].type = 0;//Squashed
                        tr_pipeline2[0].type = 0;//Squashed
                        //printf("\n\n%d\n%d\n\n",tr_pipeline2[1].type, tr_pipeline2[2].type);
                    }
                    
                }else {
                    
                    int index = tr_pipeline2[2].PC;
                    index = index >> 4;
                    index = index & 511;
                    
                    if(branch_predictor[index] == 0) {
                        
                        if (tr_pipeline2[1].PC != (tr_pipeline2[2].PC + 4)) {
                            tr_pipeline2[1].type = 0; //Squashed
                            tr_pipeline2[0].type = 0; //Squashed
                            branch_predictor[index] = 1;
                        }
                    }
                    
                    else {
                        
                        if (tr_pipeline2[2].Addr != tr_pipeline2[1].PC) {
                            tr_pipeline2[1].type = 0;//Squashed
                            tr_pipeline2[0].type = 0;//Squashed
                            branch_predictor[index] = 0;
                        }
                    }
                }
            }

            
        }
        
        // SIMULATION OF A SINGLE CYCLE cpu IS TRIVIAL - EACH INSTRUCTION IS EXECUTED
        // IN ONE CYCLE
        
        if (trace_view_on == 1) {/* print the executed instruction if trace_view_on=1 */
            
            //Pipeline 1 - ALU and branch/Jump Instructions
            switch(tr_pipeline1[5].type) {
                case ti_NOP:
                    printf("[cycle %d] NOP:\n",cycle_number) ;
                    break;
                case ti_RTYPE:
                    printf("[cycle %d] RTYPE:",cycle_number) ;
                    printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_pipeline1[5].PC, tr_pipeline1[5].sReg_a, tr_pipeline1[5].sReg_b, tr_pipeline1[5].dReg);
                    break;
                case ti_ITYPE:
                    printf("[cycle %d] ITYPE:",cycle_number) ;
                    printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_pipeline1[1].PC, tr_pipeline1[5].sReg_a, tr_pipeline1[5].dReg, tr_pipeline1[5].Addr);
                    break;
                case ti_LOAD:
                    printf("[cycle %d] LOAD:",cycle_number) ;
                    printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_pipeline1[5].PC, tr_pipeline1[5].sReg_a, tr_pipeline1[5].dReg, tr_pipeline1[5].Addr);
                    break;
                case ti_STORE:
                    printf("[cycle %d] STORE:",cycle_number) ;
                    printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_pipeline1[5].PC, tr_pipeline1[5].sReg_a, tr_pipeline1[5].sReg_b, tr_pipeline1[5].Addr);
                    break;
                case ti_BRANCH:
                    printf("[cycle %d] BRANCH:",cycle_number) ;
                    printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_pipeline1[5].PC, tr_pipeline1[5].sReg_a, tr_pipeline1[5].sReg_b, tr_pipeline1[5].Addr);
                    break;
                case ti_JTYPE:
                    printf("[cycle %d] JTYPE:",cycle_number) ;
                    printf(" (PC: %x)(addr: %x)\n", tr_pipeline1[5].PC,tr_pipeline1[5].Addr);
                    break;
                case ti_SPECIAL:
                    printf("[cycle %d] SPECIAL:\n",cycle_number) ;
                    break;
                case ti_JRTYPE:
                    printf("[cycle %d] JRTYPE:",cycle_number) ;
                    printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_pipeline1[5].PC, tr_pipeline1[5].dReg, tr_pipeline1[5].Addr);
                    break;
            }
            
            //Pipeline 2 - Load/store instructions
            switch(tr_pipeline2[5].type) {
                case ti_NOP:
                    printf("\tNOP:\n") ;
                    break;
                case ti_RTYPE:
                    printf("\tRTYPE:") ;
                    printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_pipeline2[5].PC, tr_pipeline2[5].sReg_a, tr_pipeline2[5].sReg_b, tr_pipeline2[5].dReg);
                    break;
                case ti_ITYPE:
                    printf("\tITYPE:") ;
                    printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_pipeline2[5].PC, tr_pipeline2[5].sReg_a, tr_pipeline2[5].dReg, tr_pipeline2[5].Addr);
                    break;
                case ti_LOAD:
                    printf("\tLOAD:") ;
                    printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_pipeline2[5].PC, tr_pipeline2[5].sReg_a, tr_pipeline2[5].dReg, tr_pipeline2[5].Addr);
                    break;
                case ti_STORE:
                    printf("\tSTORE:") ;
                    printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_pipeline2[5].PC, tr_pipeline2[5].sReg_a, tr_pipeline2[5].sReg_b, tr_pipeline2[5].Addr);
                    break;
                case ti_BRANCH:
                    printf("\tBRANCH:") ;
                    printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_pipeline2[5].PC, tr_pipeline2[5].sReg_a, tr_pipeline2[5].sReg_b, tr_pipeline2[5].Addr);
                    break;
                case ti_JTYPE:
                    printf("\t   JTYPE:") ;
                    printf(" (PC: %x)(addr: %x)\n", tr_pipeline2[5].PC,tr_pipeline2[5].Addr);
                    break;
                case ti_SPECIAL:
                    printf("\tSPECIAL:\n") ;
                    break;
                case ti_JRTYPE:
                    printf("\tJRTYPE:") ;
                    printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_pipeline2[5].PC, tr_pipeline2[5].dReg, tr_pipeline2[5].Addr);
                    break;
            }
            
        }
    }
    
    trace_uninit();
    free(branch_predictor);
    free(instruction_buffer);
    
    exit(0);
}

void insert_NOP(struct trace_item **item, int stage) {
    
    temp.type = ti_NOP;
    item[stage] = &temp;
    return;
    
    
}

void shift(struct trace_item **item) {
    
    int j;
    for(j = 5; j >= 0; j--) {
        (*item)[j] = (*item)[j-1];
    }
    
    // for(j = 1; j < 5; j++) {
    //     (*item)[j] = (*item)[j+1];
    // }
    
    return;
    
}

int checkPipeline(struct trace_item **item) {
    
    int x = 0;
    
    int j;
    for(j = 0; j < 5; j++) {
        switch((*item)[j].type) {
            case ti_NOP:
                x++;
                break;
        }
        
    }
    
    if (x == 5) {
        
        return 0;
        
    }
    
    return 1;
    
}

//Changes the instruction at risk for data hazard with a no-op
void hazard_switch(struct trace_item **item) {
    (*item)[0] = (*item)[1];
    (*item)[1].type = ti_NOP;
}



/*
 Checks for a possible data hazard by comparing the current instruction to
 the next one, and checking to see if a load instruction will affect something
 that follows it by changing the source of the next instruction.
 */
int check_data_hazard(struct trace_item **item){
    
    /*
     If the next instruction is an R-Type instruction and the current is a load
     instruction, where the source of the next = the destination of the current,
     stall.
     */
    if ((*item)[0].type == 1 && (*item)[1].type == 3) {
        if ((*item)[0].sReg_a == (*item)[1].dReg) {return 1;}
        /*
         Otherwise, if the second source of the next operation is the destination
         register of the previous operation, stall.
         */
        else if ((*item)[0].sReg_b == (*item)[1].dReg) {return 1;}
    }
    
    /*
     If the next instruction is an I-Type and the current instruction is a load
     instruction, AND the source register of the I-Type is the destination
     register of the Load instruction, return 1 to insert no-ops and stall.
     */
    else if ((*item)[0].type == 2 && (*item)[1].type == 3) {
        if ((*item)[0].sReg_a == (*item)[1].dReg) {return 1;}
    }
    
    /*
     A store instruction follows a load instruction, where the source relies
     on the previous destination.
     */
    else if ((*item)[0].type == 4 && (*item)[1].type == 3) {
        if ((*item)[0].sReg_a == (*item)[1].dReg) {return 1;}
    }
    
    /*
     A branch instruction follows a load instruction where the branch relies on
     the value of the load word destination
     */
    else if ((*item)[0].type == 5 && (*item)[1].type == 3){
        if ((*item)[0].sReg_a == (*item)[1].dReg) {return 1;}
        else if ((*item)[0].sReg_b == (*item)[1].dReg) {return 1;}
    }
    
    /*
     If none of these cases are met, there is no data hazard. Return 0 so that
     no stall takes place
     */
    else {return 0;}
    
    //Fallthrough case shouldn't be reached
    return 0;
}
