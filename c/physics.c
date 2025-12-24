#include "physics.h"
#include <math.h>
#include <pthread.h>
#include <stdlib.h>

// Thread function - processes bodies from start_idx to end_idx
static void* compute_forces_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    body_t* current = data->current;
    body_t* next = data->next;
    int n = data->n;
    double dt = data->dt;
    double G = data->G;
    double softening = data->softening;
    
    for (int i = data->start_idx; i < data->end_idx; i++) {
        // Start with net force = 0 in x and y
        double Fx = 0.0;
        double Fy = 0.0;
        
        // Get current body properties
        double m_i = current[i].m;
        double vx_old = current[i].vx;
        double vy_old = current[i].vy;
        double x_old = current[i].x;
        double y_old = current[i].y;
        
        // Loop over every body j
        for (int j = 0; j < n; j++) {
            // Skip j == i
            if (j == i) {
                continue;
            }
            
            // Compute dx and dy
            double dx = current[j].x - current[i].x;
            double dy = current[j].y - current[i].y;
            
            // Distance squared: dist2 = dx*dx + dy*dy + softening*softening
            double dist2 = dx * dx + dy * dy + softening * softening;
            
            // Distance: dist = sqrt(dist2)
            double dist = sqrt(dist2);
            
            // Gravitational force magnitude: F = G * m_i * m_j / dist2
            double m_j = current[j].m;
            double F = G * m_i * m_j / dist2;
            
            // Force components
            Fx += F * (dx / dist);
            Fy += F * (dy / dist);
        }
        
        // Compute acceleration
        double ax = Fx / m_i;
        double ay = Fy / m_i;
        
        // Update velocity (Euler): vx_new = vx_old + ax * dt
        double vx_new = vx_old + ax * dt;
        double vy_new = vy_old + ay * dt;
        
        // Update position: x_new = x_old + vx_new * dt
        double x_new = x_old + vx_new * dt;
        double y_new = y_old + vy_new * dt;
        
        // Store x_new, y_new, vx_new, vy_new, and m into next[i]
        next[i].x = x_new;
        next[i].y = y_new;
        next[i].vx = vx_new;
        next[i].vy = vy_new;
        next[i].m = m_i;  // Mass doesn't change
    }
    
    return NULL;
}

// function to perform one step of the simulation using multiple threads
void step_simulation(body_t* current, body_t* next, int n, double dt, double G, double softening, int num_threads) {
    // Limit threads to number of bodies
    if (num_threads > n) {
        num_threads = n;
    }
    if (num_threads < 1) {
        num_threads = 1;
    }
    
    // Stack-allocated thread handles and data
    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];
    
    // Calculate bodies per thread
    int bodies_per_thread = n / num_threads;
    int remainder = n % num_threads;
    
    int start_idx = 0;
    for (int t = 0; t < num_threads; t++) {
        // distribute remainder across first few threads
        int end_idx = start_idx + bodies_per_thread + (t < remainder ? 1 : 0);
        
        // Set up thread data
        thread_data[t].current = current;
        thread_data[t].next = next;
        thread_data[t].n = n;
        thread_data[t].dt = dt;
        thread_data[t].G = G;
        thread_data[t].softening = softening;
        thread_data[t].start_idx = start_idx;
        thread_data[t].end_idx = end_idx;
        
        pthread_create(&threads[t], NULL, compute_forces_thread, &thread_data[t]);
        
        start_idx = end_idx;
    }
    
    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }
}
