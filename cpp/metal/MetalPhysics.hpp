#pragma once

#include "../basic/Physics.hpp"

class MetalPhysics {
public:
    MetalPhysics();
    ~MetalPhysics();
    
    bool initialize();
    void step_simulation_metal(body_t* current, body_t* next, int n, double dt, double G, double softening);
    void cleanup();
    bool is_available() const { return initialized; }

private:
    void* device;  // id<MTLDevice>
    void* commandQueue;  // id<MTLCommandQueue>
    void* library;  // id<MTLLibrary>
    void* computePipelineState;  // id<MTLComputePipelineState>
    void* currentBuffer;  // id<MTLBuffer>
    void* nextBuffer;  // id<MTLBuffer>
    void* constantsBuffer;  // id<MTLBuffer>
    
    bool initialized;
    int currentBufferSize;
    void ensure_buffers_size(int n);
};
