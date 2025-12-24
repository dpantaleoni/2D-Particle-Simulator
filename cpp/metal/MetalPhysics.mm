#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#include "MetalPhysics.hpp"
#include <simd/simd.h>

struct Body {
    simd_float2 position;
    simd_float2 velocity;
    float mass;
};

MetalPhysics::MetalPhysics()
    : device(nil), commandQueue(nil), library(nil), computePipelineState(nil)
    , currentBuffer(nil), nextBuffer(nil), constantsBuffer(nil)
    , initialized(false), currentBufferSize(0)
{
}

MetalPhysics::~MetalPhysics() {
    cleanup();
}

bool MetalPhysics::initialize() {
    if (initialized) return true;
    
    @autoreleasepool {
        id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
        if (!dev) return false;
        device = (__bridge void*)dev;
        CFRetain((__bridge CFTypeRef)dev);  // Retain since MTLCreateSystemDefaultDevice returns autoreleased
        
        id<MTLCommandQueue> queue = [dev newCommandQueue];
        if (!queue) return false;
        commandQueue = (__bridge void*)queue;  // newCommandQueue already returns retained
        
        // Compile shader from source
        const char* shaderSrc = R"(
#include <metal_stdlib>
using namespace metal;
struct Body { float2 position, velocity; float mass; };
struct Constants { float dt, G, softening; uint n; };
kernel void step_simulation(device const Body* current [[buffer(0)]], device Body* next [[buffer(1)]],
    constant Constants& c [[buffer(2)]], uint i [[thread_position_in_grid]]) {
    if (i >= c.n) return;
    float m_i = current[i].mass;
    float2 pos_i = current[i].position, vel_i = current[i].velocity, F = 0;
    for (uint j = 0; j < c.n; j++) {
        if (j == i) continue;
        float2 dx = current[j].position - pos_i;
        float dist2 = dot(dx, dx) + c.softening * c.softening;
        F += c.G * m_i * current[j].mass / dist2 * (dx / sqrt(dist2));
    }
    float2 accel = F / m_i, vel_new = vel_i + accel * c.dt;
    next[i] = {pos_i + vel_new * c.dt, vel_new, m_i};
})";
        
        NSError* error = nil;
        NSString* source = [NSString stringWithUTF8String:shaderSrc];
        id<MTLLibrary> lib = [dev newLibraryWithSource:source options:nil error:&error];
        if (!lib) return false;
        library = (__bridge void*)lib;
        
        id<MTLFunction> func = [lib newFunctionWithName:@"step_simulation"];
        if (!func) return false;
        
        id<MTLComputePipelineState> pipeline = [dev newComputePipelineStateWithFunction:func error:&error];
        if (!pipeline) return false;
        computePipelineState = (__bridge void*)pipeline;
        
        id<MTLBuffer> constBuf = [dev newBufferWithLength:sizeof(float) * 3 + sizeof(uint) options:MTLResourceStorageModeShared];
        constantsBuffer = (__bridge void*)constBuf;
    }
    
    initialized = true;
    return true;
}

void MetalPhysics::ensure_buffers_size(int n) {
    if (currentBufferSize >= n) return;
    
    id<MTLDevice> dev = (__bridge id<MTLDevice>)device;
    
    if (currentBuffer) CFRelease(currentBuffer);
    if (nextBuffer) CFRelease(nextBuffer);
    
    size_t size = n * sizeof(Body);
        id<MTLBuffer> buf1 = [dev newBufferWithLength:size options:MTLResourceStorageModeShared];
    id<MTLBuffer> buf2 = [dev newBufferWithLength:size options:MTLResourceStorageModeShared];
    currentBuffer = (__bridge void*)buf1;  // newBufferWithLength already returns retained
    nextBuffer = (__bridge void*)buf2;
    currentBufferSize = n;
}

void MetalPhysics::step_simulation_metal(body_t* current, body_t* next, int n, double dt, double G, double softening) {
    ensure_buffers_size(n);
    
    Body* currentData = (Body*)[(__bridge id<MTLBuffer>)currentBuffer contents];
    Body* nextData = (Body*)[(__bridge id<MTLBuffer>)nextBuffer contents];
    
    // Copy to GPU
    for (int i = 0; i < n; i++) {
        currentData[i].position = simd_make_float2(current[i].x, current[i].y);
        currentData[i].velocity = simd_make_float2(current[i].vx, current[i].vy);
        currentData[i].mass = current[i].m;
    }
    
    // Set constants
    float* c = (float*)[(__bridge id<MTLBuffer>)constantsBuffer contents];
    c[0] = dt; c[1] = G; c[2] = softening;
    *(uint*)(c + 3) = n;
    
    // Run GPU compute shader
    id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)commandQueue;
    id<MTLCommandBuffer> cmdBuf = [queue commandBuffer];
    id<MTLComputeCommandEncoder> enc = [cmdBuf computeCommandEncoder];
    id<MTLComputePipelineState> pipeline = (__bridge id<MTLComputePipelineState>)computePipelineState;
    
    [enc setComputePipelineState:pipeline];
    [enc setBuffer:(__bridge id<MTLBuffer>)currentBuffer offset:0 atIndex:0];
    [enc setBuffer:(__bridge id<MTLBuffer>)nextBuffer offset:0 atIndex:1];
    [enc setBuffer:(__bridge id<MTLBuffer>)constantsBuffer offset:0 atIndex:2];
    
    MTLSize gridSize = MTLSizeMake((n + 255) / 256 * 256, 1, 1);
    MTLSize threadgroupSize = MTLSizeMake(256, 1, 1);
    [enc dispatchThreads:gridSize threadsPerThreadgroup:threadgroupSize];
    [enc endEncoding];
    [cmdBuf commit];
    [cmdBuf waitUntilCompleted];
    
    // Copy results back
    for (int i = 0; i < n; i++) {
        next[i].x = nextData[i].position.x;
        next[i].y = nextData[i].position.y;
        next[i].vx = nextData[i].velocity.x;
        next[i].vy = nextData[i].velocity.y;
        next[i].m = nextData[i].mass;
    }
}

void MetalPhysics::cleanup() {
    if (currentBuffer) { CFRelease(currentBuffer); currentBuffer = nil; }
    if (nextBuffer) { CFRelease(nextBuffer); nextBuffer = nil; }
    if (constantsBuffer) { CFRelease(constantsBuffer); constantsBuffer = nil; }
    if (computePipelineState) { CFRelease(computePipelineState); computePipelineState = nil; }
    if (library) { CFRelease(library); library = nil; }
    if (commandQueue) { CFRelease(commandQueue); commandQueue = nil; }
    if (device) { CFRelease(device); device = nil; }
    initialized = false;
    currentBufferSize = 0;
}

