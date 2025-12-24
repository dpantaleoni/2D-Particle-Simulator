#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <thread>
#include <stdexcept>

#include "raylib.h"
#include "Init.hpp"
#include "Physics.hpp"
#include "../metal/MetalPhysics.hpp"

const int N_STEPS = 1000;
const double DT = 0.01;
const double G = 1.0;
const double SOFTENING = 0.1; // Increased to prevent extreme forces when particles get close

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const double VIEW_RANGE = 10.0; // View from -VIEW_RANGE to +VIEW_RANGE

int main(int argc, char* argv[]) {
    bool use_gpu = false;
    int n;
    
    // Parse command line arguments
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <number_of_particles> [--gpu]" << std::endl;
        std::cerr << "Example: " << argv[0] << " 100" << std::endl;
        std::cerr << "Example: " << argv[0] << " 100 --gpu" << std::endl;
        return 1;
    }
    
    // Parse number of particles
    try {
        n = std::stoi(argv[1]);
    } catch (const std::exception& e) {
        std::cerr << "Error: invalid number of particles" << std::endl;
        return 1;
    }
    
    if (n <= 0) {
        std::cerr << "Error: number of particles must be positive" << std::endl;
        return 1;
    }
    
    // Check for GPU flag
    if (argc == 3) {
        std::string flag = argv[2];
        if (flag == "--gpu" || flag == "-gpu") {
            use_gpu = true;
        }
    }

    // Initialize Metal if GPU mode is requested
    MetalPhysics* metalPhysics = nullptr;
    if (use_gpu) {
        metalPhysics = new MetalPhysics();
        if (!metalPhysics->initialize()) {
            std::cerr << "Warning: Metal GPU initialization failed, falling back to CPU" << std::endl;
            delete metalPhysics;
            metalPhysics = nullptr;
            use_gpu = false;
        } else {
            std::cout << "Using Metal GPU acceleration" << std::endl;
        }
    }
    
    int num_threads = static_cast<int>(std::thread::hardware_concurrency());
    if (!use_gpu) {
        std::cout << "Using " << num_threads << " CPU threads" << std::endl;
    }
    if (num_threads < 1) {
        num_threads = 1;
    }
    
    // Allocate two vectors
    std::vector<body_t> current(n);
    std::vector<body_t> next(n);

    // Initialize only current with init_bodies
    init_bodies(current.data(), n);
    
    // Initialize raylib window
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "2D Particle Simulation");
    SetTargetFPS(60); // Limit to 60 FPS for visible animation
    
    // Start timing
    auto start = std::chrono::steady_clock::now();
    
    int step = 0;
    // Main loop: advance simulation one step per frame
    while (step < N_STEPS && !WindowShouldClose()) {
        // Call step_simulation (CPU or GPU)
        if (use_gpu && metalPhysics) {
            metalPhysics->step_simulation_metal(current.data(), next.data(), n, DT, G, SOFTENING);
        } else {
            step_simulation(current.data(), next.data(), n, DT, G, SOFTENING, num_threads);
        }
        
        // Swap current and next vectors
        std::swap(current, next);
        step++;
        
        // Render particles (raylib will handle frame timing)
        BeginDrawing();
        ClearBackground(BLACK);
        
        // Map simulation coordinates to screen coordinates
        // Simulation: [-VIEW_RANGE, VIEW_RANGE] -> Screen: [0, WINDOW_WIDTH/HEIGHT]
        double scale_x = WINDOW_WIDTH / (2.0 * VIEW_RANGE);
        double scale_y = WINDOW_HEIGHT / (2.0 * VIEW_RANGE);
        double offset_x = WINDOW_WIDTH / 2.0;
        double offset_y = WINDOW_HEIGHT / 2.0;
        
        // Draw each particle
        for (int i = 0; i < n; i++) {
            int screen_x = static_cast<int>(current[i].x * scale_x + offset_x);
            int screen_y = static_cast<int>(current[i].y * scale_y + offset_y);
            
            // Only draw if particle is within view bounds
            if (screen_x >= 0 && screen_x < WINDOW_WIDTH && 
                screen_y >= 0 && screen_y < WINDOW_HEIGHT) {
                DrawCircle(screen_x, screen_y, 2.0f, PURPLE);
            }
        }
        
        EndDrawing();
    }
    
    // Stop timing
    auto end = std::chrono::steady_clock::now();
    
    // Calculate elapsed time in seconds
    auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
    double elapsed = duration.count();
    
    // Print results
    std::cout << "Elapsed time: " << std::fixed << std::setprecision(6) << elapsed << " seconds" << std::endl;
    
    // Keep window open until user closes it
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);
        
        // Draw final state
        double scale_x = WINDOW_WIDTH / (2.0 * VIEW_RANGE);
        double scale_y = WINDOW_HEIGHT / (2.0 * VIEW_RANGE);
        double offset_x = WINDOW_WIDTH / 2.0;
        double offset_y = WINDOW_HEIGHT / 2.0;
        
        for (int i = 0; i < n; i++) {
            int screen_x = static_cast<int>(current[i].x * scale_x + offset_x);
            int screen_y = static_cast<int>(current[i].y * scale_y + offset_y);
            
            if (screen_x >= 0 && screen_x < WINDOW_WIDTH && 
                screen_y >= 0 && screen_y < WINDOW_HEIGHT) {
                DrawCircle(screen_x, screen_y, 2.0f, PURPLE);
            }
        }
        
        EndDrawing();
    }
    
    // Clean up raylib
    CloseWindow();
    
    // Clean up Metal resources
    if (metalPhysics) {
        delete metalPhysics;
    }
    
    return 0;
}