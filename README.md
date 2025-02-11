# Memory Stress Test

This project demonstrates a multi-threaded memory stress test program in C++ designed to saturate the memory bus and measure throughput (MB/s). It can run in either sequential or random access modes, allowing you to observe differences in performance and potential bottlenecks related to the von Neumann architecture (where data and instruction share the same bus).

## Table of Contents

1. [Overview](#overview)  
2. [Requirements](#requirements)  
3. [Build & Run](#build&run)   
4. [Configuration](#configuration)  
5. [Experimenting Further](#experimenting-further)  
6. [What the Test Does](#what-the-test-does)  

---

## Overview

- **Multi-threaded**: Spawns multiple threads to read from and write to a shared large buffer, which puts pressure on the memory subsystem.  
- **Sequential vs. Random Access**: You can toggle between two memory-access patterns via a command-line argument (`--random` or `-r`).  
- **Measurement**: Reports total bytes processed, elapsed time, and throughput (in MB/s).  
- **Demonstrates**: The limitations of the shared memory bus (the von Neumann bottleneck).  

---

## Requirements

- A modern C++ compiler supporting at least C++17 (e.g., `g++`, `clang++`, MSVC).  
- A machine with enough memory to handle the chosen buffer size (default is 512 MB).  

---

## Build & Run

1. **Clone or download** this repository.  
2. **Open a terminal** in the directory containing the `main.cpp` file.  
3. **Compile** with optimizations for best results, for example using `g++`:
   ```bash
   g++ -std=c++17 -O3 main.cpp -o main
   ```
   This produces an executable named `main`.

> **Note**: Using `-O3` or `-O2` optimization flags can significantly affect throughput measurements.

From the terminal, run:

```bash
./main
```

This runs the program in **sequential access** mode by default. To enable **random access** mode, use:

```bash
./main --random
```
or
```bash
./main -r
```

### Example Output

```text
Memory Stress Test
------------------
Buffer size    : 536870912 bytes
Iterations     : 10
Threads        : 8
Access pattern : Sequential

Total bytes processed : 10737418240.00 bytes
Elapsed time          : 1.25 s
Throughput            : 8179.87 MB/s
```

---

## Configuration

You can modify these constants in `main.cpp` to change behavior:

- **`NUM_THREADS`**: Number of threads (default: 8)  
- **`BUFFER_SIZE`**: Size of the large buffer (default: 512 MB). You can switch to 1 GB or 2 GB by adjusting:
  ```cpp
  static const size_t BUFFER_SIZE = 1024ull * 1024ull * 1024ull; // 1 GB
  ```
- **`ITERATIONS`**: Number of times each thread repeats the read/write pattern.  

---

## Experimenting Further

1. **Change `NUM_THREADS`**:  
   - Observe how throughput scales as you add more threads. Initially, performance may increase, but beyond a certain point, you’ll likely see diminishing returns due to the memory bus becoming saturated.

2. **Adjust `BUFFER_SIZE`**:  
   - Try 256 MB, 512 MB, 1 GB, 2 GB, etc. Once the buffer is larger than the last-level cache (LLC), random accesses often incur more cache misses.

3. **Random vs. Sequential**:  
   - Compare throughput in sequential mode vs. random mode (`--random`). You’ll likely see lower throughput with random accesses due to less effective prefetching and more cache misses.

4. **Test on Different Hardware**:  
   - Run on machines with different CPU, memory speed, and cache sizes. Compare how the memory bandwidth limits differ.

---

## What the Test Does

1. **Initial Setup**:  
   - Allocates a large buffer (`std::vector<char>`) of size `BUFFER_SIZE`.  
   - Spawns `NUM_THREADS` threads, each assigned a chunk of the buffer.

2. **Memory Operations**:  
   - Each thread runs a loop for `ITERATIONS`.  
   - **Read Phase**: It reads all bytes in its chunk (either sequentially or in a random order) and sums them into a `volatile` variable to prevent compiler optimization.  
   - **Write Phase**: It writes a simple pattern (the current index as a `char`) to every element in its chunk.

3. **Throughput Calculation**:  
   - Each thread reports the total bytes processed (`2 * chunk_size` per iteration—one for read, one for write).  
   - The main thread sums these totals in an atomic variable and divides by elapsed time (in seconds) to get bytes/s.  
   - Finally, it converts bytes/s to MB/s (`1 MB = 1024 * 1024` bytes) and prints results.

By running this test, you can witness the **von Neumann bottleneck** in action. Even though CPUs can do many operations in parallel, they share the same memory bus to fetch data (and instructions). As you scale up threads or use random access, you put more pressure on the memory subsystem, eventually hitting a throughput ceiling determined by the hardware’s maximum memory bandwidth.
