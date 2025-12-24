#include "Init.hpp"
#include <cstdlib>
#include <ctime>

void init_bodies(body_t* bodies, int n) {
    srand(time(NULL));
    // create random values for each body_t in the array
    double pos_max = 1;
    double pos_min = -1;
    double vel_max = 0.1;
    double vel_min = -0.1;
    double mass_max = 5.0;
    double mass_min = 0.5;
    
    for (int i = 0; i < n; i++) {
        bodies[i].x = pos_min + (pos_max - pos_min) * ((double)rand() / RAND_MAX);  
        bodies[i].y = pos_min + (pos_max - pos_min) * ((double)rand() / RAND_MAX);
        bodies[i].vx = vel_min + (vel_max - vel_min) * ((double)rand() / RAND_MAX);
        bodies[i].vy = vel_min + (vel_max - vel_min) * ((double)rand() / RAND_MAX);
        bodies[i].m = mass_min + (mass_max - mass_min) * ((double)rand() / RAND_MAX);        
    }
}