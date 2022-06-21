#ifndef MCMC_IO_H
#define MCMC_IO_H

// Struct that stores ILI data read from file.
typedef struct {
    size_t size;  // Number of elements in each array.
    int *year;    // Year data was collected
    int *week;    // Week of the year data was collected
    int *estInc;  // Estimated incidence for H1pdm
    int *fluSeason; // Pointer for beginning of the influenza season
    int fluDuration; // Number of weeks during flu season
} ILIinput;

int read_ili_csv(const char* fname, ILIinput* data_p);
void free_ili_input(ILIinput* data_p);

int read_csv_double_vector(const char* fname, double* *vec_p, int* vsize_p);

#endif
