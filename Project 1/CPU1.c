/**************************************************************/
/* CS/COE 1541
 Noah Perryman
 Patrick Lyons
 just compile with gcc -o CPU CPU.c
 and execute using
 ./CPU 'trace file' 'prediction_method' 'trace_vew_on'
 ***************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "CPU.h"

char *stage_ID_map(int i);
int checkPipeline(struct trace_item **item);
void insert_NOP(struct trace_item **item, int stage);
void shift(struct trace_item **item);
struct trace_item temp;

int main(int argc, char **argv)
{
    struct trace_item *tr_entry; //create a new trace_item struct pointer
    struct trace_item *tr_pipeline;
    size_t size;
    char *trace_file_name;
    int prediction_method = 0; //branch prediction on or off. 0 for predict not taken, 1 for 1-bit prediction. Default value is 0
    int trace_view_on = 0; //Cycle output printed to screen on or off. 0 for off, 1 for on
    
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
    
    //Initialize Pipeline with 5 no_ops
    int k;
    for(k = 0; k < 5; k++) {
        insert_NOP(&tr_pipeline, k);
    }
    
    int *branch_predictor = malloc(sizeof(int)*64);
    for (k = 0; k < 64; k++) {
        branch_predictor[k] = 0;
    }
    
    while(1) { //Execute the trace program
        size = trace_get_item(&tr_entry); //get size of the trace program as well as the trace_item field
        
        if (!size && !checkPipeline(&tr_pipeline)) {       /* no more instructions (trace_items) to simulate */
            printf("+ Simulation terminates at cycle : %u\n", cycle_number);
            break;
        }
        else{              /* parse the next instruction to simulate */
            cycle_number++;
            /*t_type = tr_entry->type;
            t_sReg_a = tr_entry->sReg_a;
            t_sReg_b = tr_entry->sReg_b;
            t_dReg = tr_entry->dReg;
            t_PC = tr_entry->PC;
            t_Addr = tr_entry->Addr;*/
            shift(&tr_pipeline);
            if (!size) { //insert into pipeline
                insert_NOP(&tr_pipeline, 0);
            }else {
                tr_pipeline[0] = *tr_entry;
            }
            
            //see if branch or jump first
            if (tr_pipeline[2].type == ti_BRANCH || tr_pipeline[2].type == ti_JTYPE || tr_pipeline[2].type == ti_JRTYPE) {
                
                if (prediction_method == 0) {
                                 
                    if (tr_pipeline[1].PC != (tr_pipeline[2].PC + 4)) {
                        
                        printf("\n\n%x\n%x\n\n",tr_pipeline[1].PC, tr_pipeline[2].PC);
                        insert_NOP(&tr_pipeline, 1);
                        insert_NOP(&tr_pipeline, 0);
                       
                    }
                                 
                }else {
                    
                    int index = tr_pipeline[2].PC;
                    index = index >> 4;
                    index = index & 511;
                    
                    if(branch_predictor[index] == 0) {
                        
                        if (tr_pipeline[1].PC != (tr_pipeline[2].PC + 4)) {
                            
                            insert_NOP(&tr_pipeline, 1);
                            insert_NOP(&tr_pipeline, 0);
                            branch_predictor[index] = 1;
                            
                        }
                        
                    }else {
                        
                        if (tr_pipeline[2].Addr != tr_pipeline[1].PC) {
                            
                            insert_NOP(&tr_pipeline, 1);
                            insert_NOP(&tr_pipeline, 0);
                            branch_predictor[index] = 0;
                            
                        }
                        
                    }
                                 
                }
                
            }
            
        }
        
        // SIMULATION OF A Pipelined CPU
        
        if (trace_view_on) {/* print the executed instruction if trace_view_on=1 */
            
            int i;
            for(i = 0; i < 5; i++){
                char *temp = stage_ID_map(i);
                switch (i) {
                    case 0:
                        switch(tr_pipeline[i].type) {
                            case ti_NOP:
                                printf("[cycle %d] [%s Stage] NOP:\n",cycle_number, temp) ;
                                break;
                            case ti_RTYPE:
                                printf("[cycle %d] [%s Stage] RTYPE:",cycle_number, temp) ;
                                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_pipeline[i].PC, tr_pipeline[i].sReg_a, tr_pipeline[i].sReg_b, tr_pipeline[i].dReg);
                                break;
                            case ti_ITYPE:
                                printf("[cycle %d] [%s Stage] ITYPE:",cycle_number, temp) ;
                                printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_pipeline[i].PC, tr_pipeline[i].sReg_a, tr_pipeline[i].dReg, tr_pipeline[i].Addr);
                                break;
                            case ti_LOAD:
                                printf("[cycle %d] [%s Stage] LOAD:",cycle_number, temp) ;
                                printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_pipeline[i].PC, tr_pipeline[i].sReg_a, tr_pipeline[i].dReg, tr_pipeline[i].Addr);
                                break;
                            case ti_STORE:
                                printf("[cycle %d] [%s Stage] STORE:",cycle_number, temp) ;
                                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_pipeline[i].PC, tr_pipeline[i].sReg_a, tr_pipeline[i].sReg_b, tr_pipeline[i].Addr);
                                break;
                            case ti_BRANCH:
                                printf("[cycle %d] [%s Stage] BRANCH:",cycle_number, temp) ;
                                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_pipeline[i].PC, tr_pipeline[i].sReg_a, tr_pipeline[i].sReg_b, tr_pipeline[i].Addr);
                                break;
                            case ti_JTYPE:
                                printf("[cycle %d] [%s Stage] JTYPE:",cycle_number, temp) ;
                                printf(" (PC: %x)(addr: %x)\n", tr_pipeline[i].PC,tr_pipeline[i].Addr);
                                break;
                            case ti_SPECIAL:
                                printf("[cycle %d] [%s Stage] SPECIAL\n:",cycle_number, temp) ;
                                break;
                            case ti_JRTYPE:
                                printf("[cycle %d] [%s Stage] JRTYPE:",cycle_number, temp) ;
                                printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_pipeline[i].PC, tr_pipeline[i].dReg, tr_pipeline[i].Addr);
                                break;
                        }
                        break;
                    default:
                        switch(tr_pipeline[i].type) {
                            case ti_NOP:
                                printf("\t\t[%s Stage] NOP:\n", temp) ;
                                break;
                            case ti_RTYPE:
                                printf("\t\t[%s Stage] RTYPE:", temp) ;
                                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_pipeline[i].PC, tr_pipeline[i].sReg_a, tr_pipeline[i].sReg_b, tr_pipeline[i].dReg);
                                break;
                            case ti_ITYPE:
                                printf("\t\t[%s Stage] ITYPE:", temp) ;
                                printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_pipeline[i].PC, tr_pipeline[i].sReg_a, tr_pipeline[i].dReg, tr_pipeline[i].Addr);
                                break;
                            case ti_LOAD:
                                printf("\t\t[%s Stage] LOAD:", temp) ;
                                printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_pipeline[i].PC, tr_pipeline[i].sReg_a, tr_pipeline[i].dReg, tr_pipeline[i].Addr);
                                break;
                            case ti_STORE:
                                printf("\t\t[%s Stage] STORE:", temp) ;
                                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_pipeline[i].PC, tr_pipeline[i].sReg_a, tr_pipeline[i].sReg_b, tr_pipeline[i].Addr);
                                break;
                            case ti_BRANCH:
                                printf("\t\t[%s Stage] BRANCH:", temp) ;
                                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_pipeline[i].PC, tr_pipeline[i].sReg_a, tr_pipeline[i].sReg_b, tr_pipeline[i].Addr);
                                break;
                            case ti_JTYPE:
                                printf("\t\t[%s Stage] JTYPE:", temp) ;
                                printf(" (PC: %x)(addr: %x)\n", tr_pipeline[i].PC,tr_pipeline[i].Addr);
                                break;
                            case ti_SPECIAL:
                                printf("\t\t[%s Stage] SPECIAL:\n", temp) ;
                                break;
                            case ti_JRTYPE:
                                printf("\t\t[%s Stage] RTYPE:", temp) ;
                                printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_pipeline[i].PC, tr_pipeline[i].dReg, tr_pipeline[i].Addr);
                                break;
                        }
                        break;
                }
            }
        }
    }
    
    trace_uninit(); //uninitialize the trace and exit
    free(branch_predictor);
    
    exit(0);
}

char *stage_ID_map(int i) {
    
    if(i == 0) {
        return "IF";
    }else if(i == 1) {
        return "ID";
    }else if(i == 2) {
        return "EX";
    }else if(i == 3) {
        return "MEM";
    }else {
        return "WB";
    }
    
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

void insert_NOP(struct trace_item **item, int stage) {

    temp.type = ti_NOP;
    item[stage] = &temp;
    return;
    
}

void shift(struct trace_item **item) {
    
    int j;
    for(j = 4; j > 0; j--) {
        (*item)[j] = (*item)[j-1];
    }
    
    return;
    
}
