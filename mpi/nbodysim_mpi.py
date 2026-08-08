from mpi4py import MPI
import numpy as np
import math, random, sys
from time import time as timer

N_STEPS = 100
DT = 0.01
G = 1.0
SOFTENING = 1e-3

def main():
    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    size = comm.Get_size()

    n = int(sys.argv[1]) # number of bodies

    bodies = np.zeros((n, 5), dtype=np.float64)
    forces = np.zeros((n, 2), dtype=np.float64)

    # Only rank 0 initializes
    if rank == 0:
        ts = timer()
        init_bodies(bodies, n)

    # broadcast initial bodies states to all ranks
    comm.Bcast(bodies, root=0)

    # divide bodies for distribution
    counts = [(n // size + (1 if i < n % size else 0)) for i in range(size)]
    offsets = [sum(counts[:i]) for i in range(size)]
    my_start = offsets[rank]
    my_end = my_start + counts[rank]

    for _ in range(N_STEPS):
        # Each rank computes forces for its slice
        compute_forces_local(bodies, forces, my_start, my_end, n, G, SOFTENING)

        # Allreduce to sum partial forces across all ranks
        global_forces = np.zeros_like(forces)
        comm.Allreduce(forces, global_forces, op=MPI.SUM)

        # Every rank updates positions/velocities (keeps bodies in sync)
        for i in range(n):
            ax = global_forces[i, 0] / bodies[i, 4]
            ay = global_forces[i, 1] / bodies[i, 4]
            bodies[i, 2] += ax * DT
            bodies[i, 3] += ay * DT
            bodies[i, 0] += bodies[i, 2] * DT
            bodies[i, 1] += bodies[i, 3] * DT

    if rank == 0:
        te = timer()
        t = te - ts
        print(t)
        # print(f"Elapsed time: {end_time - start_time:.6f} seconds")

def compute_forces_local(bodies, forces, start, end, n, G, softening):
    # Each rank computes forces only for bodies[start:end]
    for i in range(start, end):
        fx, fy = 0.0, 0.0
        xi, yi, mi = bodies[i, 0], bodies[i, 1], bodies[i, 4]

        for j in range(n):
            if j == i:
                continue
            dx = bodies[j, 0] - xi
            dy = bodies[j, 1] - yi
            dist2 = dx * dx + dy * dy + softening * softening
            dist = math.sqrt(dist2)
            F = G * mi * bodies[j, 4] / dist2
            fx += F * (dx / dist)
            fy += F * (dy / dist)

        forces[i, 0] = fx
        forces[i, 1] = fy

def init_bodies(bodies, n):
    random.seed(timer())
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


if __name__ == "__main__":
    main()