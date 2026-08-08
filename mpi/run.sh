#!/bin/bash

mpirun -np 6 \
  --hostfile hostfile.txt \
  --mca btl tcp,self \
  --mca btl_tcp_if_include enp1s0 \
  --mca oob_tcp_if_include enp1s0 \
  --mca plm_rsh_no_tree_spawn 1 \
  python3 $@

