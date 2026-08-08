from pyspark.sql import SparkSession
import numpy as np
import math, random, sys
from time import time as timer

N_STEPS = 100
DT = 0.01
G = 1.0
SOFTENING = 1e-3


def main():

    n = int(sys.argv[1])

    bodies = np.zeros((n, 5), dtype=np.float64)
    spark = SparkSession.builder.appName("nbodysim_spark").getOrCreate()
    sc = spark.sparkContext
    sc.setLogLevel("ERROR")

    size = sc.defaultParallelism
    counts = [(n // size + (1 if i < n % size else 0)) for i in range(size)]
    offsets = [sum(counts[:i]) for i in range(size)]
    ranges = [(offsets[i], offsets[i] + counts[i]) for i in range(size)]

    ranges_rdd = sc.parallelize(ranges, numSlices=len(ranges))

    ts = timer()
    
    init_bodies(bodies, n)

    for _ in range(N_STEPS):
        # broadcast current bodies to workers
        bodies_bc = sc.broadcast(bodies)

        # each partition computes forces for its slice
        partial_forces_rdd = ranges_rdd.map( 
            lambda start_end: compute_forces_local( 
                bodies_bc.value, start_end[0], start_end[1], n, G, SOFTENING
            ) 
        )

        # reduce to sum partial forces across all workers
        global_forces = partial_forces_rdd.reduce(lambda a, b: a + b)

        # update positions/velocities on driver (keeps bodies in sync)
        for i in range(n):
            ax = global_forces[i, 0] / bodies[i, 4]
            ay = global_forces[i, 1] / bodies[i, 4]
            bodies[i, 2] += ax * DT
            bodies[i, 3] += ay * DT
            bodies[i, 0] += bodies[i, 2] * DT
            bodies[i, 1] += bodies[i, 3] * DT

        bodies_bc.destroy()  # destroy to free memory for next iteration

    te = timer()
    t = te - ts
    print(t)
    spark.stop()


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


def compute_forces_local(bodies, start, end, n, G, softening):
    # each worker computes forces only for bodies[start:end]
    forces = np.zeros((n, 2), dtype=np.float64)

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

    return forces


if __name__ == "__main__":
    main()