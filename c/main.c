#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "init.h"
#include "physics.h"

const int N_STEPS = 1000;
const double DT = 0.01;
const double G = 1.0;
const double SOFTENING = 1e-3;

int main (int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_particles>\n", argv[0]);
        fprintf(stderr, "Example: %s 100\n", argv[0]);
        return 1;
    }
    
    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "Error: number of particles must be positive\n");
        return 1;
    }

    int num_threads = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (num_threads < 1) {
        num_threads = 1;
    }
    
    // Allocate two arrays
    body_t* current = malloc(n * sizeof(body_t));
    body_t* next = malloc(n * sizeof(body_t));
    
    if (current == NULL || next == NULL) {
        fprintf(stderr, "failed to allocate mem to bodies\n");
        if (current != NULL) free(current);
        if (next != NULL) free(next);
        return 1;
    }

    // Initialize only current with init_bodies
    init_bodies(current, n);
    
    // Start timing
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Loop for some number of steps
    for (int step = 0; step < N_STEPS; step++) {
        // Call step_simulation
        step_simulation(current, next, n, DT, G, SOFTENING, num_threads);
        
        // Swap current and next pointers
        body_t* temp = current;
        current = next;
        next = temp;
    }
    
    // Stop timing
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    // Calculate elapsed time in seconds
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    // Print results
    printf("Elapsed time: %.6f seconds\n", elapsed);
    
    // Free both arrays
    free(current);
    free(next);
    
    return 0;
}