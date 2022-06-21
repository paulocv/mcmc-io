/*
Read and store ILI data from a csv file.

User manual of libcsv.
https://github.com/rgamble/libcsv/blob/master/csv.pdf
*/

#include <stdio.h>
#include <stdlib.h>
#include "mcmc_io.h"


int main (int argc, char *argv[])
{
  char* fname;
  char* contacts_fname;
  ILIinput data = {};

  double* contacts = NULL;
  int t = 0;

  // Check the arguments given to the program call
  if (argc < 3) {
    fprintf(stderr, "Please inform the csv file names (2 in total: ILI data and contacts) as an arugment.\n");
    return EXIT_FAILURE;
  }
  fname = argv[1];
  contacts_fname = argv[2];

  // -------------- USE THESE CHUNKS IN YOUR CODE ----------------
  // Read the ILI file. Return if reading is unsuccessful.
  if (read_ili_csv(fname, &data)){
    // [[ Please perform free/deallocations here ]]
    return EXIT_FAILURE;
  }

  // Read the contacts file. Return if unsuccessful.
  if (read_csv_double_vector(contacts_fname, &contacts, &t)){
    // [[ Please perform free/deallocations here ]]
    return EXIT_FAILURE;
  }

  // -----------------------------------------------------------

  // Print ILI content
  for (int i = 0; i < data.size; i++){
    printf("%d, %d, %d\n", data.year[i], data.week[i], data.estInc[i]);
  }
  printf("Data has %lu entries.\n", data.size);

  // Print contacts.csv content
  for (int i = 0; i < t; i++){
    printf("%lf\n", contacts[i]);
  }
  printf("Data has %d entries.\n", t);


  // Free contacts data
  free(contacts);

  // Free ILI data with interface function.
  free_ili_input(&data);

  return EXIT_SUCCESS;
}
 
