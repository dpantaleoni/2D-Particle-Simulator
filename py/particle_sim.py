import random
import math
import time
import multiprocessing as mp
import multiprocessing.shared_memory as shm
import numpy as np


def init_bodies(bodies, n):
    random.seed(time.time())
    pos_max = 1.0
    pos_min = -1.0
    vel_max = 0.1
    vel_min = -0.1
    mass_max = 5.0
    mass_min = 0.5
    
    for i in range(n):
        bodies[i, 0] = pos_min + (pos_max - pos_min) * random.random()  # x
        bodies[i, 1] = pos_min + (pos_max - pos_min) * random.random()  # y
        bodies[i, 2] = vel_min + (vel_max - vel_min) * random.random()  # vx
        bodies[i, 3] = vel_min + (vel_max - vel_min) * random.random()  # vy
        bodies[i, 4] = mass_min + (mass_max - mass_min) * random.random()  # m


def compute_forces_worker(start, end, n, bodies_name, forces_name, G, softening):
    shm_b = shm.SharedMemory(name=bodies_name)
    shm_f = shm.SharedMemory(name=forces_name)
    
    bodies = np.ndarray((n, 5), dtype=np.float64, buffer=shm_b.buf)
    forces = np.ndarray((n, 2), dtype=np.float64, buffer=shm_f.buf)
    
    for i in range(start, end):
        fx = 0.0
        fy = 0.0
        
        xi = bodies[i, 0]
        yi = bodies[i, 1]
        mi = bodies[i, 4]
        
        for j in range(n):
            if j == i:
                continue
            
            dx = bodies[j, 0] - xi
            dy = bodies[j, 1] - yi
            
            dist2 = dx * dx + dy * dy + softening * softening
            dist = math.sqrt(dist2)
            
            mj = bodies[j, 4]
            F = G * mi * mj / dist2
            
            fx += F * (dx / dist)
            fy += F * (dy / dist)
        
        forces[i, 0] = fx
        forces[i, 1] = fy
    
    shm_b.close()
    shm_f.close()


def step_simulation(sh_bodies, sh_forces, shm_bodies, shm_forces, n, dt, G, softening, num_procs):
    if num_procs > n:
        num_procs = n
    if num_procs < 1:
        num_procs = 1
    
    sh_forces[:] = 0.0
    
    processes = []
    bodies_per_proc = n // num_procs
    remainder = n % num_procs
    
    for w in range(num_procs):
        start = w * bodies_per_proc
        end = n if w == num_procs - 1 else (w + 1) * bodies_per_proc
        
        p = mp.Process(
            target=compute_forces_worker,
            args=(start, end, n, shm_bodies.name, shm_forces.name, G, softening)
        )
        p.start()
        processes.append(p)
    
    for p in processes:
        p.join()
    
    for i in range(n):
        ax = sh_forces[i, 0] / sh_bodies[i, 4]
        ay = sh_forces[i, 1] / sh_bodies[i, 4]
        
        sh_bodies[i, 2] += ax * dt
        sh_bodies[i, 3] += ay * dt
        sh_bodies[i, 0] += sh_bodies[i, 2] * dt
        sh_bodies[i, 1] += sh_bodies[i, 3] * dt


N_STEPS = 1000
DT = 0.01
G = 1.0
SOFTENING = 1e-3


def main():
    import sys
    
    if len(sys.argv) != 2:
        print("Usage: {} <number_of_particles>".format(sys.argv[0]), file=sys.stderr)
        print("Example: {} 100".format(sys.argv[0]), file=sys.stderr)
        sys.exit(1)
    
    try:
        n = int(sys.argv[1])
        if n <= 0:
            print("Error: number of particles must be positive", file=sys.stderr)
            sys.exit(1)
    except ValueError:
        print("Error: invalid number of particles", file=sys.stderr)
        sys.exit(1)
    
    try:
        mp.set_start_method('fork', force=True)
    except RuntimeError:
        pass  # Use default start method
    
    num_procs = mp.cpu_count()
    if num_procs < 1:
        num_procs = 1
    
    bodies = np.zeros((n, 5), dtype=np.float64)
    forces = np.zeros((n, 2), dtype=np.float64)
    
    init_bodies(bodies, n)
    
    shm_bodies = shm.SharedMemory(create=True, size=bodies.nbytes)
    shm_forces = shm.SharedMemory(create=True, size=forces.nbytes)
    
    sh_bodies = np.ndarray(bodies.shape, dtype=bodies.dtype, buffer=shm_bodies.buf)
    sh_forces = np.ndarray(forces.shape, dtype=forces.dtype, buffer=shm_forces.buf)
    
    sh_bodies[:] = bodies
    sh_forces[:] = 0.0
    
    start_time = time.time()
    
    for step in range(N_STEPS):
        step_simulation(sh_bodies, sh_forces, shm_bodies, shm_forces, n, DT, G, SOFTENING, num_procs)
    
    elapsed = time.time() - start_time
    
    shm_bodies.close()
    shm_bodies.unlink()
    shm_forces.close()
    shm_forces.unlink()
    
    print("Elapsed time: {:.6f} seconds".format(elapsed))


if __name__ == "__main__":
    main()
