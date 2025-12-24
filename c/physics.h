#ifndef PHYSICS_H
#define PHYSICS_H

// Body structure
typedef struct {
    double x; // position
    double y;
    double vx; // velocity
    double vy; 
    double m; // mass
} body_t;

// Thread data structure
typedef struct {
    body_t* current; // pointer to the current state of the bodies
    body_t* next; // pointer to the next state of the bodies
    int n; // number of bodies  
    double dt; // time step
    double G; // gravitational constant
    double softening; // softening parameter
    int start_idx; // starting index of the bodies to process
    int end_idx; // ending index of the bodies to process
} thread_data_t;

void step_simulation(body_t* current, body_t* next, int n, double dt, double G, double softening, int num_threads);

#endif