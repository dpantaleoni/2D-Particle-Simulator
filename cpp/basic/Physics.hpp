#pragma once

// Body structure
typedef struct {
    double x; // position
    double y;
    double vx; // velocity
    double vy; 
    double m; // mass
} body_t;

void step_simulation(body_t* current, body_t* next, int n, double dt, double G, double softening, int num_threads);
