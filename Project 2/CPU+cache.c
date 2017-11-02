/**************************************************************/
/* CS/COE 1541
 Noah Perryman
 Patrick Lyons
 just compile with gcc -o CPU+cache CPU+cache.c
 and execute using
 ./CPU+cache 'trace file' 'prediction_method' 'trace_vew_on'
 ***************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "CPU.h"
#include "cache.h"

int checkPipeline();
void insert_NOP(int stage);
void shift();
void insert_squashed(int k);
int predict_branch(int PC);
int check_data_hazard();
void print_pipeline_output(int cycle, int stalled);

//to keep values for pipelined CPU
struct trace_item tr_pipeline[6];
int read_next = 1;
int branch_op = 0;
int branch_predictor[64];
struct trace_item *item;

// to keep cache statistics
unsigned int I_accesses = 0;
unsigned int I_misses = 0;
unsigned int D_read_accesses = 0;
unsigned int D_read_misses = 0;
unsigned int D_write_accesses = 0;
unsigned int D_write_misses = 0;

int main(int argc, char **argv)
{

    size_t size;
    char *trace_file_name;
    char *cache_config_name;
    int prediction_method = 0; //branch prediction on or off. 0 for predict not taken, 1 for 1-bit prediction. Default value is 0
    int trace_view_on = 0; //Cycle output printed to screen on or off. 0 for off, 1 for on
    static FILE *cache_config_trace;
    int stall = 0;

    //values for trace item
    unsigned char t_type = 0;
    unsigned char t_sReg_a= 0;
    unsigned char t_sReg_b= 0;
    unsigned char t_dReg= 0;
    unsigned int t_PC = 0;
    unsigned int t_Addr = 0;

    //initial values for cache parameters before taking input
    unsigned int I_size = 16;
    unsigned int I_assoc = 4;
    unsigned int I_bsize = 8;
    unsigned int D_size = 16;
    unsigned int D_assoc = 4;
    unsigned int D_bsize = 8;
    unsigned int mem_latency = 20;

    int k;
    for(k = 0; k < 5; k++) {
        insert_NOP(k);
    }

    unsigned int cycle_number = 0;

    if (argc == 1) { //If the input does not include the filename there is nothing to trace (i.e. no input), so exit
        fprintf(stdout, "\nUSAGE: tv <trace_file> <switch - any character>\n");
        fprintf(stdout, "\n(switch) to turn on or off individual item view.\n\n");
        exit(0);
    }

    if (argc != 5 && argc != 4) {
        fprintf(stdout, "\nWrong input arguments\n\n");
        exit(0);
    }

    //Get file inputs
    trace_file_name = argv[1];
    prediction_method = atoi(argv[2]);
    trace_view_on = atoi(argv[3]);
    if (argc == 5) {
        cache_config_name = argv[4];
    }else {
        cache_config_name = "cache_config.txt";
    }

    //open file
    cache_config_trace = fopen(cache_config_name, "rb");
    if (!cache_config_trace) { //if file fopen() returns an error, exit
        fprintf(stdout, "\ncache_config file %s not opened.\n\n", cache_config_name);
        exit(0);
    }
    //read in file values
    fscanf(cache_config_trace, "%d", &I_size);
    fscanf(cache_config_trace, "%d", &I_assoc);
    fscanf(cache_config_trace, "%d", &I_bsize);
    fscanf(cache_config_trace, "%d", &D_size);
    fscanf(cache_config_trace, "%d", &D_assoc);
    fscanf(cache_config_trace, "%d", &D_bsize);
    fscanf(cache_config_trace, "%d", &mem_latency);
    fclose(cache_config_trace);

    //printf("%d\n%d\n%d\n%d\n%d\n%d\n%d\n",I_size,I_assoc,I_bsize,D_size,D_assoc,D_bsize,mem_latency);

    if (prediction_method < 0 || prediction_method > 1 || trace_view_on < 0 || trace_view_on > 1) {
            fprintf(stdout, "\nInvalid input arguments\n\n");
            exit(0);
    }

    fprintf(stdout, "\n ** opening file %s\n", trace_file_name);


    trace_fd = fopen(trace_file_name, "rb"); //open file to be read

    if (!trace_fd) { //if file fopen() returns an error, exit
        fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
        exit(0);
    }

    printf("\nCache Size: %d KB", I_size);
    printf("\nAssociativity: %d", I_assoc);
    printf("\nBlock Size: %d Bytes\n\n", I_bsize);

    trace_init(); //initialize the trace
    struct cache_t *I_cache, *D_cache;
    I_cache = cache_create(I_size, I_bsize, I_assoc, mem_latency);
    D_cache = cache_create(D_size, D_bsize, D_assoc, mem_latency);

    for (k = 0; k < 64; k++) {
        branch_predictor[k] = 0;
    }

    while(1) { //Execute the trace program{
        if(read_next == 1 && branch_op == 0) {
            size = trace_get_item(&item); //get size of the trace program as well as the trace_item field
            /*Insert code for retrieving instruction cache here in IF stage
             If fetching instruction misses, stall pipeline for stall_cycles*/
            int stall_cycle = 0;
            stall_cycle = cache_access(I_cache,item->PC,0);
            I_accesses++;
            if (stall_cycle != 0) {
                I_misses++;
            }
            for (k = 0; k < stall_cycle; k++) {
                cycle_number++;
                if (trace_view_on == 1) {
                    print_pipeline_output(cycle_number, 1);
                }
            }

        }

        if (!size && !checkPipeline()) {       /* no more instructions (trace_items) to simulate */
            printf("+ Simulation terminates at cycle : %u\n", cycle_number);
            printf("I-cache accesses %u and misses %u\n", I_accesses, I_misses);
            printf("D-cache Read accesses %u and misses %u\n", D_read_accesses, D_read_misses);
            printf("D-cache Write accesses %u and misses %u\n", D_write_accesses, D_write_misses);
            printf("\n\n*************************************************");

            printf("\n\nI-cache miss rate = %.6f\n", (float)((float)I_misses/(float)I_accesses));
            printf("D-cache miss rate: %.6f\n", (float)(((float)D_read_misses+D_write_misses)/((float)D_read_accesses+D_write_accesses)));
            /*printf("I-cache Miss Rate (percent): %.3f\n", (float)((float)I_misses/(float)I_accesses)*100.0);
            printf("D-cache Read Miss Rate (percent): %.3f\n", (float)((float)D_read_misses/(float)D_read_accesses)*100.0);
            printf("D-cache Write Miss Rate (percent): %.3f\n", (float)((float)D_write_misses/(float)D_write_accesses)*100.0);
            printf("D-cache: %.3f\n", (float)(((float)D_read_misses+D_write_misses)/((float)D_read_accesses+D_write_accesses))*100.0);
            printf("Overal Miss Rate (percent): %.3f\n", (float)(((float)I_misses+D_read_misses+D_write_misses)/((float)I_accesses+D_read_accesses+D_write_accesses))*100.0);*/


            break ;
        }
        else{              /* parse the next instruction to simulate */
            cycle_number++;
            shift();

            /*Before checking pipeline for hazards, insert code for data cache here in MEM stage
            If load/store to data cache misses, stall entire pipeline for stall_cycles*/
            if (tr_pipeline[4].type == ti_LOAD || tr_pipeline[4].type == ti_STORE) {
                int stall_cycle = 0;
                if (tr_pipeline[4].type == ti_LOAD) {
                    stall_cycle = cache_access(D_cache,tr_pipeline[4].Addr,0);
                    D_read_accesses++;
                    if (stall_cycle != 0) {
                        D_read_misses++;
                    }
                }else {
                    stall_cycle = cache_access(D_cache,tr_pipeline[4].Addr,1);
                    D_write_accesses++;
                    if (stall_cycle != 0) {
                        D_write_misses++;
                    }
                }

             for (k = 0; k < stall_cycle; k++) {
                     cycle_number++;
                     if (trace_view_on == 1) {
                         print_pipeline_output(cycle_number, 1);
                     }

                 }
            }

            if (check_data_hazard()) {

                insert_NOP(0);
                read_next = 0;

            }else if ((branch_op == 0) && (tr_pipeline[1].type == ti_BRANCH)) { //see if branch or jump first

                if(prediction_method == 1) {
                    if (predict_branch(tr_pipeline[1].PC) != 0) {
                        if (predict_branch(tr_pipeline[1].PC) != item->PC) {

                            int index = tr_pipeline[1].PC;
                            index = index >> 4;
                            index = index & 511;
                            branch_predictor[index] = 0; //update branch if incorrect branch

                            insert_squashed(0);
                            branch_op++;

                        }else {
                            read_next = 1;
                            if (!size) { //insert into pipeline
                                insert_NOP(0);
                            }else {
                                tr_pipeline[0] = *item;
                            }
                        }

                    }else {

                        if (item->PC != (tr_pipeline[1].PC + 4)) {

                            int index = tr_pipeline[1].PC;
                            index = index >> 4;
                            index = index & 511;
                            branch_predictor[index] = tr_pipeline[1].Addr; //update branch if taken and there was no prediction

                            insert_squashed(0);
                            branch_op++;

                        }else {
                            read_next = 1;
                            if (!size) { //insert into pipeline
                                insert_NOP(0);
                            }else {
                                tr_pipeline[0] = *item;
                            }
                        }
                    }

                }else{

                    if (item->PC != (tr_pipeline[1].PC + 4)) {
                        insert_squashed(0);
                        branch_op++;
                    }else {
                        read_next = 1;
                        if (!size) { //insert into pipeline
                            insert_NOP(0);
                        }else {
                            tr_pipeline[0] = *item;
                        }
                    }

                }

              }else if (branch_op != 0) {

                  branch_op++;
                  insert_squashed(0);

                  if (branch_op == 2) {
                      branch_op = 0;
                  }

                  read_next = 0;

              }else {
                read_next = 1;
                if (!size) { //insert into pipeline
                    insert_NOP(0);
                }else {
                    tr_pipeline[0] = *item;
                }
            }
        }

        // SIMULATION OF A Pipelined CPU

        //cycle_number = cycle_number + cache_access(I_cache, item->PC, 0); /* simulate instruction fetch */
        // update I_access and I_misses

        //prints what left the pipeline
        if (trace_view_on == 1) {
            print_pipeline_output(cycle_number, 0);
        }

    }

    trace_uninit(); //uninitialize the trace and exit

    exit(0);
}

int checkPipeline() {

    int x = 0;

    int j;
    for(j = 0; j < 5; j++) {
        switch((tr_pipeline)[j].type) {
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

void insert_NOP(int stage) {

    // Init a new no-op to insert in the pipeline
    struct trace_item no_op;
    no_op.type = 0;
    no_op.sReg_a = 0;
    no_op.sReg_b = 0;
    no_op.dReg = 0;
    no_op.PC = 0;
    no_op.Addr = 0;
    tr_pipeline[stage] = no_op;
    return;


}

void shift() {

    int j;
    for(j = 5; j >= 0; j--) {
        (tr_pipeline)[j] = (tr_pipeline)[j-1];
    }

    // for(j = 1; j < 5; j++) {
    //     (tr_pipeline)[j] = (tr_pipeline)[j+1];
    // }

    return;

}

/*
 Checks for a possible data hazard by comparing the current instruction to
 the next one, and checking to see if a load instruction will affect something
 that follows it by changing the source of the next instruction.
 */
int check_data_hazard(){

    /*
     If the next instruction is an R-Type instruction and the current is a load
     instruction, where the source of the next = the destination of the current,
     stall.
     */
    if (item->type == 1 && (tr_pipeline)[1].type == 3) {
        if (item->sReg_a == (tr_pipeline)[1].dReg) {return 1;}
        /*
         Otherwise, if the second source of the next operation is the destination
         register of the previous operation, stall.
         */
        else if (item->sReg_b == (tr_pipeline)[1].dReg) {return 1;}
    }

    /*
     If the next instruction is an I-Type and the current instruction is a load
     instruction, AND the source register of the I-Type is the destination
     register of the Load instruction, return 1 to insert no-ops and stall.
     */
    else if (item->type == 2 && (tr_pipeline)[1].type == 3) {
        if (item->sReg_a == (tr_pipeline)[1].dReg) {return 1;}
    }

    /*
     A store instruction follows a load instruction, where the source relies
     on the previous destination.
     */
    else if (item->type == 4 && (tr_pipeline)[1].type == 3) {
        if (item->sReg_a == (tr_pipeline)[1].dReg) {return 1;}
    }

    /*
     A branch instruction follows a load instruction where the branch relies on
     the value of the load word destination
     */
    else if (item->type == 5 && (tr_pipeline)[1].type == 3){
        if (item->sReg_a == (tr_pipeline)[1].dReg) {return 1;}
        else if (item->sReg_b == (tr_pipeline)[1].dReg) {return 1;}
    }

    /*
     If none of these cases are met, there is no data hazard. Return 0 so that
     no stall takes place
     */
    else {return 0;}

    //Fallthrough case shouldn't be reached
    return 0;
}

void insert_squashed(int k){

    // Init a new squashed instruction to insert in the pipeline
    struct trace_item squashed_inst;
    squashed_inst.type = 0;
    squashed_inst.sReg_a = 0;
    squashed_inst.sReg_b = 0;
    squashed_inst.dReg = 0;
    squashed_inst.PC = 0;
    squashed_inst.Addr = -1;

    tr_pipeline[k] = squashed_inst;

    return;
}

int predict_branch(int PC) {
    int index = PC;
    index = index >> 4;
    index = index & 511;
    return branch_predictor[index];
}

void print_pipeline_output(int cycle_number, int stalled) {

    if (stalled == 1) {
        printf("[cycle %d] Pipeline Stalled Due to Cache Miss\n",cycle_number);
    }else {
        switch(tr_pipeline[5].type) {
            case ti_NOP:
                if(tr_pipeline[5].Addr == -1) {
                    printf("[cycle %d] SQUASHED\n",cycle_number);
                }else {
                    printf("[cycle %d] NOP\n",cycle_number);
                }
                break;
            case ti_RTYPE:
                printf("[cycle %d] RTYPE:",cycle_number) ;
                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_pipeline[5].PC, tr_pipeline[5].sReg_a, tr_pipeline[5].sReg_b, tr_pipeline[5].dReg);
                break;
            case ti_ITYPE:
                printf("[cycle %d] ITYPE:",cycle_number) ;
                printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_pipeline[5].PC, tr_pipeline[5].sReg_a, tr_pipeline[5].dReg, tr_pipeline[5].Addr);
                break;
            case ti_LOAD:
                printf("[cycle %d] LOAD:",cycle_number) ;
                printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_pipeline[5].PC, tr_pipeline[5].sReg_a, tr_pipeline[5].dReg, tr_pipeline[5].Addr);
                break;
            case ti_STORE:
                printf("[cycle %d] STORE:",cycle_number) ;
                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_pipeline[5].PC, tr_pipeline[5].sReg_a, tr_pipeline[5].sReg_b, tr_pipeline[5].Addr);
                break;
            case ti_BRANCH:
                printf("[cycle %d] BRANCH:",cycle_number) ;
                printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_pipeline[5].PC, tr_pipeline[5].sReg_a, tr_pipeline[5].sReg_b, tr_pipeline[5].Addr);
                break;
            case ti_JTYPE:
                printf("[cycle %d] JTYPE:",cycle_number) ;
                printf(" (PC: %x)(addr: %x)\n", tr_pipeline[5].PC,tr_pipeline[5].Addr);
                break;
            case ti_SPECIAL:
                printf("[cycle %d] SPECIAL\n:",cycle_number) ;
                break;
            case ti_JRTYPE:
                printf("[cycle %d] JRTYPE:",cycle_number) ;
                printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_pipeline[5].PC, tr_pipeline[5].dReg, tr_pipeline[5].Addr);
                break;
        }
    }
}
