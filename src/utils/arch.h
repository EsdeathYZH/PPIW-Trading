#pragma once

// you can use `lscpu` to check the CPU information

// Core(s) per socket
#define PER_SOCKET_CORES  12    // number of (physical) cores per socket

// Socket(s)
#define NUM_SOCKETS       2     // number of sockets

// NUMA node0 CPU(s)
#define SOCKET_0 \
  0,2,4,6,8,10,12,14,16,18,20,22

// NUMA node1 CPU(s)
#define SOCKET_1 \
  1,3,5,7,9,11,13,15,17,19,21,23

#define CPUS \
  {{SOCKET_0}, {SOCKET_1}}
  // {{SOCKET_0}}

// choose RDMA NIC id according to sockets
// you can use 0 if you only have one NIC
#define CHOOSE_NIC(core_id) \
  ((core_id > PER_SOCKET_CORES)? 0 : 1)
