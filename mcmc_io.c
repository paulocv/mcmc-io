/* 
Toolset for data input/output (io) for the Influenza MCMC project.

v1.01 (2022-06-07) – Removes the requirement for exactly 4 columns. Columns to the right are ignored.

Version history
v1.00 (2022-06-01) – First release. Dynamically expands the vector as data is read. Checks for data 
   integrity while parsing (overflow, number of fields, conversion to integer).

v0.01 (2022-05-26) – Development version. Performs the basic reading without complete error checking.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "libcsv/csv.h"

#include "mcmc_io.h"

#define FILE_NUM_COLS 4  // Expected number of columns in the file.
#define FILE_BUF_SIZE 1024  // Size, in bytes, of the chunks of the file that are read at each input operation.


// ------------------------------------------------------------------------------------------------
// AUXILIARY STRUCTS AND FUNCTIONS
// ------------------------------------------------------------------------------------------------

// Auxiliary struct with extra variables to help on the file parsing.
typedef struct ILIinputAux{
    ILIinput* data_p;

    size_t capacity;  // Assured number of allocated positions in each vector.
    size_t curr_row;  // Row index (1-based) currently being read.
    size_t curr_col;  // Column index (1-based) currently being read.
    
    int err_status;   // Error code for inner parsing errors (i.e., during callback)
    char* err_field;  // Stores the value of the faulty field.

} ILIinputAux;


#define PARSE_EINVALID 4  // Update this number when new error codes are included.
static char *parse_errors[] = 
    /* 0 */{"success",
    /* 1 */ "could not convert string to int",
    /* 2 */ "value is out of range for int",
    /* 3 */ "line has too many fields",
    /* 4 */ "previous line has not enough fields",
    /*...*/ "invalid status code"};


char* cb_err_str(int err_status){
    if (err_status >= PARSE_EINVALID || err_status < 0){
        return parse_errors[PARSE_EINVALID];
    }
    else{
        return parse_errors[err_status];
    }
}


int alloc_ili_input(ILIinput* data_p, size_t reserve_size){
    data_p->year =   (int*) malloc(reserve_size * sizeof(int));
    data_p->week =   (int*) malloc(reserve_size * sizeof(int));
    data_p->estInc = (int*) malloc(reserve_size * sizeof(int));
    data_p->size = 0;

    if (!data_p->year || !data_p->week || !data_p->estInc){
        fprintf(stderr, "Failed to allocate ILIinput struct data.");
        return 1;
    }
    return 0;
}


int realloc_ili_input(ILIinput* data_p, size_t new_capacity){

    data_p->year =   (int*) realloc(data_p->year, new_capacity * sizeof(int));
    data_p->week =   (int*) realloc(data_p->week, new_capacity * sizeof(int));
    data_p->estInc = (int*) realloc(data_p->estInc, new_capacity * sizeof(int));

    if (!data_p->year || !data_p->week || !data_p->estInc){
        fprintf(stderr, "Failed to reallocate ILIinput struct data.");
        return 1;
    }
    return 0;
}


/*
Frees the dynamically allocated arrays for the ILIinput struct. 
Sets its pointers to NULL and its size to 0.
*/
void free_ili_input(ILIinput* data_p){
    free(data_p->year); data_p->year = NULL;
    free(data_p->week); data_p->week = NULL;
    free(data_p->estInc); data_p->estInc = NULL;
    data_p->size = 0;
}


static int is_space(unsigned char c) {
    if (c == CSV_SPACE || c == CSV_TAB) return 1;
    return 0;
}

static int is_term(unsigned char c) {
    if (c == CSV_CR || c == CSV_LF) return 1;
    return 0;
}


int parse_int_error_check(const char* str, int *err_p, size_t len){
    /*
    Parses a string into an integer with multiple error checks:
    - Overflow 
    - Invalid characters (even if they come after valid ones)
    */

    char *cursor;
    long tmp;
    long pdif;

    // Parse to long int (10 = base)
    tmp = strtol(str, &cursor, 10);

    // Check if reading succeeded and if it ended at expected size
    pdif = cursor - str;  // Number of characters effectively parsed.
    if (pdif != len){
        *err_p = 1;  // Could not convert string to int
        return 0;
    }

    // Check for overflow (both for long and for int)
    if (errno == ERANGE || tmp < INT_MIN || tmp > INT_MAX){
        *err_p = 2;  // 
        return 0;
    }

    return (int) tmp;
}


double parse_double_error_check(const char* str, int *err_p, size_t len){
    // Pass
    // TODO
}



// ------------------------------------------------------------------------------------------------
// PARSING CALLBACK FUNCTIONS
// ------------------------------------------------------------------------------------------------

/* 
Callback function for each field that is read from file.
*/
void cb1 (void *s_v, size_t len, void *aux_vp) { 

    // Convert void pointers to meaningful types
    char* s = (char*) s_v;
    ILIinputAux* aux_p = (ILIinputAux*) aux_vp;
    ILIinput* data_p = aux_p->data_p;

    if (aux_p->curr_row == 1) return;  // Ignore first row of the file
    if (aux_p->err_status) return;  // Do not parse if an error occurred before

    switch (aux_p->curr_col){
    case 0:
    case 1:
        // DO NOTHING. These columns are ignored.
        // OBS: case 0 should not be reached, columns are 1-based.
        break;

    case 2:
        data_p->year[data_p->size] = parse_int_error_check(s, &aux_p->err_status, len);
        break;

    case 3:
        data_p->week[data_p->size] = parse_int_error_check(s, &aux_p->err_status, len);
        break;

    case 4:
        data_p->estInc[data_p->size] = parse_int_error_check(s, &aux_p->err_status, len);
        break;
    
    default:
        // aux_p->err_status = 3;  // line has too many fields (this check is not performed)
        break;
    }

    // If there was an error, registers the field for later reporting.
    if (aux_p->err_status){
        aux_p->err_field = (char*) malloc(len + 1 * sizeof(char));
        strcpy(aux_p->err_field, s);
        return;  // Prevents curr_col from being updated.
    }
    
    aux_p->curr_col++;
}


/* 
Callback function for each valid row that is read from file.
*/
void cb2 (int c, void *aux_vp) { 
    ILIinputAux* aux_p = (ILIinputAux*) aux_vp;
    ILIinput* data_p = aux_p->data_p;

    if (aux_p->err_status) return;  // Do not operate if there was a parsing error.

    // Check for the number of fields
    if (aux_p->curr_col - 1 < FILE_NUM_COLS && aux_p->curr_row > 1){  // -1 as it was previously incremented
        aux_p->err_status = 4;  // previous line has not enough fields
        aux_p->err_field = (char*) malloc(2 * sizeof(char));
        strcpy(aux_p->err_field, "");
    }

    // Update cursors
    aux_p->curr_col = 1;
    if (aux_p->curr_row++ == 1) return;  // Update current row AND ignore if it's the first one.

    data_p->size++;  // If line was valid, increments the size of data containers.

    // Dynamical vector reallocation (doubles capacity if needed).
    if (data_p->size >= aux_p->capacity){
        aux_p->capacity *= 2;
        realloc_ili_input(data_p, aux_p->capacity);
    }
}


// ------------------------------------------------------------------------------------------------
// HIGH-LEVEL INTERFACE FUNCTIONS
// ------------------------------------------------------------------------------------------------


/* 
Reads a csv file with ILI data. 

Assumes that the file has the following 4 columns:
"index", "year","week","est_Inc"

Where the first column ("index") is ignored. 
The first row of the file is assumed as index and is also ignored.


@param fname  Path for the csv file. Must be a null-terminated string.
@param data_p   Pointer to an ILIinput struct, to which the data is written.

@return An integer error code.
*/
int read_ili_csv(const char* fname, ILIinput* data_p){

    // Declarations
    // ------------
    FILE *fp;
    struct csv_parser parser;
    char buf[FILE_BUF_SIZE];
    size_t bytes_read;
    unsigned char options = 0;
    const size_t reserve_size = 53;  // Could be a parameter, but no optionals in C.
    ILIinputAux aux = {};

    // Initialization
    // --------------
  
    // Initialization of the auxiliary parser structure.
    aux.data_p = data_p;
    aux.capacity = reserve_size;
    aux.curr_row = aux.curr_col = 1;
    aux.err_status = 0;
    aux.err_field = NULL;

    // Initial allocation of the struct pointers
    alloc_ili_input(data_p, reserve_size);

    // Initialiation of the parser
    if (csv_init(&parser, options) != 0) {
        fprintf(stderr, "Failed to initialize csv parser\n");
        return EXIT_FAILURE;
    }

    // This can be ignored/commented, csvlib defines default functions for space/term inside the parser.
    csv_set_space_func(&parser, is_space);
    csv_set_term_func(&parser, is_term);

    // Set option to append null string terminator to each field
    options += CSV_APPEND_NULL;  
    csv_set_opts(&parser, options); 


    // Execution
    // ---------

    // --- File opening
    fp = fopen(fname, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open %s: \"%s\"\n", fname, strerror(errno));
        return(EXIT_FAILURE);
    }

    // --- Main loop for reading and parsing the file
    while ((bytes_read=fread(buf, 1, FILE_BUF_SIZE, fp)) > 0) {
        if (csv_parse(&parser, buf, bytes_read, cb1, cb2, &aux) != bytes_read) {
            fprintf(stderr, "Error while parsing file: \"%s\"\n", csv_strerror(csv_error(&parser)));
            break;
        }

        // Handle inner parsing error
        if (aux.err_status){
            fprintf(stderr, "Error parsing field %lu (\"%s\") of line %lu: %s\n", aux.curr_col, 
                aux.err_field, aux.curr_row, cb_err_str(aux.err_status));
            free(aux.err_field);
            break;
        }
    }

    // Final operations
    // ----------------

    // Shrink to fit the actual vector size
    if (aux.capacity > data_p->size)
        realloc_ili_input(data_p, data_p->size);

    if (ferror(fp) || aux.err_status || parser.status) {
        fprintf(stderr, "Error while reading file \"%s\"\n", fname);
        fclose(fp);
        csv_free(&parser);  // Frees the csv parser.
        return(EXIT_FAILURE);
    }

    fclose(fp);
    csv_fini(&parser, cb1, cb2, &aux);  // Closes the csv parser.
    csv_free(&parser);  // Frees the csv parser.

    return EXIT_SUCCESS;
}
