#ifndef TRACE_ITEM_H
#define TRACE_ITEM_H

// this is tpts
enum trace_item_type { //type of instruction
    ti_NOP = 0,
    ti_RTYPE,
    ti_ITYPE,
    ti_LOAD,
    ti_STORE,
    ti_BRANCH,
    ti_JTYPE,
    ti_SPECIAL,
    ti_JRTYPE
};

struct trace_item {
    unsigned char type;            // see above
    unsigned char sReg_a;            // 1st operand
    unsigned char sReg_b;            // 2nd operand
    unsigned char dReg;            // dest. operand
    unsigned int PC;            // program counter
    unsigned int Addr;            // mem. address
};

#endif

#define TRACE_BUFSIZE 1024*1024

static FILE *trace_fd;
static int trace_buf_ptr;
static int trace_buf_end;
static struct trace_item *trace_buf;
static FILE *out_fd;

int is_big_endian(void) //checks endianness of computer for data reading purposes
{
    union {
        uint32_t i;
        char c[4];
    } bint = { 0x01020304 };
    
    return bint.c[0] == 1; //Big endinness will have read in 1 at the [0] location. If this is correct, it has big endianness.
}

uint32_t my_ntohl(uint32_t x) //returns macro for assigning bits of type unit32_t
{
    u_char *s = (u_char *)&x;
    return (uint32_t)(s[3] << 24 | s[2] << 16 | s[1] << 8 | s[0]); //assign bits 24-31 to s[3], 16-23 to s[2], 8-15 to s[1], and 0-7 to s[0]. Basically, each character is one bytes
}

void trace_init() //initialize the trace
{
    trace_buf = malloc(sizeof(struct trace_item) * TRACE_BUFSIZE); //allocate memory based on the trace size, where each element is trace_item struct size
    
    if (!trace_buf) { //if memory not allocated or there is a problem with malloc, exit
        fprintf(stdout, "** trace_buf not allocated\n");
        exit(-1);
    }
    
    trace_buf_ptr = 0; //Initiate Head and tail of trace_buf items
    trace_buf_end = 0;
}

void trace_uninit() //free the memory allocated by maloc and close the trace file at the end of the program
{
    free(trace_buf);
    fclose(trace_fd);
}

int trace_get_item(struct trace_item **item) //get the trace items in the buffer. If there is nothing in the buffer, read in values. If there are no values to read in, return.
{
    int n_items;
    
    if (trace_buf_ptr == trace_buf_end) {    /* if no more unprocessed items in the trace buffer, get new data  */
        n_items = fread(trace_buf, sizeof(struct trace_item), TRACE_BUFSIZE, trace_fd);
        if (!n_items) return 0;                /* if no more items in the file, we are done */
        
        trace_buf_ptr = 0;
        trace_buf_end = n_items;            /* n_items were read and placed in trace buffer */
    }
    
    *item = &trace_buf[trace_buf_ptr];    /* read a new trace item for processing */
    trace_buf_ptr++;
    
    if (is_big_endian()) { //determine how the PC and Addr are read based on endianness
        (*item)->PC = my_ntohl((*item)->PC);
        (*item)->Addr = my_ntohl((*item)->Addr);
    }
    
    return 1;
}

int write_trace(struct trace_item item, char *fname)//write trace file
{
    out_fd = fopen(fname, "a");
    int n_items;
    if (is_big_endian()) { //determine how the PC and Addr are read based on Endianness
        (&item)->PC = my_ntohl((&item)->PC);
        (&item)->Addr = my_ntohl((&item)->Addr);
    }
    
    n_items = fwrite(&item, sizeof(struct trace_item), 1, out_fd); //determine number of items written
    fclose(out_fd); //close file
    if (!n_items) return 0;                /* if no more items in the file, we are done */
    
    
    return 1;
}


