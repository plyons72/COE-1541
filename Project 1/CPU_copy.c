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

char *stage_id_map(int i);

int main(int argc, char **argv)
{
    struct trace_item *tr_entry; //create a new trace_item struct pointer
    //struct trace_item tr_entry;
    size_t size; //
    char *trace_file_name;
    int prediction_method = 0; //branch prediction on or off. 0 for predict not taken, 1 for 1-bit prediction. Default value is 0
    int trace_view_on = 0; //Cycle output printed to screen on or off. 0 for off, 1 for on
    
    unsigned char t_type = 0;
    unsigned char t_sReg_a= 0;
    unsigned char t_sReg_b= 0;
    unsigned char t_dReg= 0;
    unsigned int t_PC = 0;
    unsigned int t_Addr = 0;
    
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
    
    while(1) { //Execute the trace program
        size = trace_get_item(&tr_entry); //get size of the trace program as well as the trace_item field
        
        if (!size) {       /* no more instructions (trace_items) to simulate */
            printf("+ Simulation terminates at cycle : %u\n", cycle_number);
            break;
        }
        else{              /* parse the next instruction to simulate */
            cycle_number++;
            t_type = tr_entry->type;
            t_sReg_a = tr_entry->sReg_a;
            t_sReg_b = tr_entry->sReg_b;
            t_dReg = tr_entry->dReg;
            t_PC = tr_entry->PC;
            t_Addr = tr_entry->Addr;
        }
        
        // SIMULATION OF A Pipelined CPU
        
        if (trace_view_on) {/* print the executed instruction if trace_view_on=1 */
            
            int i;
            for(i = -4; i < -1; i--){
                switch (i) {
                    case 5:
                        switch(tr_entry/*[i]*/->type) {
                            case ti_NOP:
                                printf("[cycle %d] NOP:",cycle_number) ;
                                break;
                            case ti_RTYPE:
                                printf("[cycle %d] [%s Stage] RTYPE:",cycle_number, stage_ID_map(i)) ;
                                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_entry[i]->PC, tr_entry[i]->sReg_a, tr_entry[i]->sReg_b, tr_entry[i]->dReg);
                                break;
                            case ti_ITYPE:
                                printf("[cycle %d] [%s Stage] ITYPE:",cycle_number, stage_ID_map(i)) ;
                                printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry[i]->PC, tr_entry[i]->sReg_a, tr_entry[i]->dReg, tr_entry[i]->Addr);
                                break;
                            case ti_LOAD:
                                printf("[cycle %d] [%s Stage] LOAD:",cycle_number, stage_ID_map(i)) ;
                                printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry[i]->PC, tr_entry[i]->sReg_a, tr_entry[i]->dReg, tr_entry[i]->Addr);
                                break;
                            case ti_STORE:
                                printf("[cycle %d] [%s Stage] STORE:",cycle_number, stage_ID_map(i)) ;
                                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry[i]->PC, tr_entry[i]->sReg_a, tr_entry[i]->sReg_b, tr_entry[i]->Addr);
                                break;
                            case ti_BRANCH:
                                printf("[cycle %d] [%s Stage] BRANCH:",cycle_number, stage_ID_map(i)) ;
                                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry[i]->PC, tr_entry[i]->sReg_a, tr_entry[i]->sReg_b, tr_entry[i]->Addr);
                                break;
                            case ti_JTYPE:
                                printf("[cycle %d] [%s Stage] JTYPE:",cycle_number, stage_ID_map(i)) ;
                                printf(" (PC: %x)(addr: %x)\n", tr_entry[i]->PC,tr_entry[i]->Addr);
                                break;
                            case ti_SPECIAL:
                                printf("[cycle %d] [%s Stage] SPECIAL:",cycle_number, stage_ID_map(i)) ;
                                break;
                            case ti_JRTYPE:
                                printf("[cycle %d] [%s Stage] JRTYPE:",cycle_number, stage_ID_map(i)) ;
                                printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_entry/*[i]*/->PC, tr_entry[i]->dReg, tr_entry[i]->Addr);
                                break;
                        }
                        break;
                    default:
                        switch(tr_entry[i]->type) {
                            case ti_NOP:
                                printf("NOP:") ;
                                break;
                            case ti_RTYPE:
                                printf("\t[%s Stage] RTYPE:", stage_ID_map(i)) ;
                                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_entry[i]->PC, tr_entry[i]->sReg_a, tr_entry[i]->sReg_b, tr_entry[i]->dReg);
                                break;
                            case ti_ITYPE:
                                printf("\t[%s Stage] ITYPE:", stage_ID_map(i)) ;
                                printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry[i]->PC, tr_entry[i]->sReg_a, tr_entry[i]->dReg, tr_entry[i]->Addr);
                                break;
                            case ti_LOAD:
                                printf("\t[%s Stage] LOAD:", stage_ID_map(i)) ;
                                printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry[i]->PC, tr_entry[i]->sReg_a, tr_entry[i]->dReg, tr_entry[i]->Addr);
                                break;
                            case ti_STORE:
                                printf("\t[%s Stage] STORE:", stage_ID_map(i)) ;
                                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry[i]->PC, tr_entry[i]->sReg_a, tr_entry[i]->sReg_b, tr_entry[i]->Addr);
                                break;
                            case ti_BRANCH:
                                printf("\t[%s Stage] BRANCH:", stage_ID_map(i)) ;
                                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry[i]->PC, tr_entry[i]->sReg_a, tr_entry[i]->sReg_b, tr_entry[i]->Addr);
                                break;
                            case ti_JTYPE:
                                printf("\t[%s Stage] JTYPE:", stage_ID_map(i)) ;
                                printf(" (PC: %x)(addr: %x)\n", tr_entry[i]->PC,tr_entry[i]->Addr);
                                break;
                            case ti_SPECIAL:
                                printf("\t[%s Stage] SPECIAL:", stage_ID_map(i)) ;
                                break;
                            case ti_JRTYPE:
                                printf("\t[%s Stage] RTYPE:", stage_ID_map(i)) ;
                                printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_entry[i]->PC, tr_entry[i]->dReg, tr_entry[i]->Addr);
                                break;
                        }
                        break;
                }
            }
        }
    }
    
    trace_uninit(); //uninitialize the trace and exit
    
    exit(0);
}

char *stage_ID_map(int i) {
    
    if(i == 5) {
        return "IF";
    }else if(i == 4) {
        return "ID";
    }else if(i == 3) {
        return "EX";
    }else if(i == 2) {
        return "MEM";
    }else {
        return "WB";
    }
    
}




