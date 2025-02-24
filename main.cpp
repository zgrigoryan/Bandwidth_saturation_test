#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>  
#include <random>    
#include <iomanip>
#include <numeric>
#include <cstring>    

// # of threads to spawn
static const int NUM_THREADS = 8;

// size of buffer in bytes (512mb)
static const size_t BUFFER_SIZE = 512ull * 1024ull * 1024ull;

// # of iterations each thread will run read/write loops
static const size_t ITERATIONS = 10;

// flag to indicate whether to randomize memory access
// (default is sequential access)
// If true, each thread will read/write to random locations
static bool useRandomAccess = false;

///////////////////////////////////////////////////////
// Data structure for thread workload
///////////////////////////////////////////////////////
struct ThreadWork {
    char* data;                     
    size_t size;                    
    std::atomic<size_t>* global_bytes_processed;
    size_t iterations;
};

void memory_stress(ThreadWork work) 
{
    char* buffer = work.data;
    size_t local_size = work.size;
    size_t iters = work.iterations;

    // when using random access, prepare a shuffled list of indices
    std::vector<size_t> indices;
    if (useRandomAccess) {
        indices.resize(local_size);
        std::iota(indices.begin(), indices.end(), 0);
        // shuffle the indices
        static thread_local std::mt19937 rng(std::random_device{}());
        std::shuffle(indices.begin(), indices.end(), rng);
    }

    for (size_t i = 0; i < iters; ++i) {
        volatile int sum = 0;

        if (useRandomAccess) {
            // random access mode
            for (size_t j = 0; j < local_size; ++j) {
                sum += buffer[ indices[j] ];
            }
        } else {
            // sequential mode
            for (size_t j = 0; j < local_size; ++j) {
                sum += buffer[j];
            }
        }


        if (useRandomAccess) {
            // random access mode
            for (size_t j = 0; j < local_size; ++j) {
                buffer[ indices[j] ] = static_cast<char>(j);
            }
        } else {
            // sequential mode
            for (size_t j = 0; j < local_size; ++j) {
                buffer[j] = static_cast<char>(j);
            }
        }

        (*work.global_bytes_processed) += (local_size * 2);
    }
}

int main(int argc, char* argv[])
{
    // check for a --random or -r command line argument
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--random") == 0 ||
            std::strcmp(argv[i], "-r") == 0) 
        {
            useRandomAccess = true;
        }
    }

    std::cout << "Memory Stress Test\n"
              << "------------------\n";
    std::cout << "Buffer size    : " << BUFFER_SIZE << " bytes\n";
    std::cout << "Iterations     : " << ITERATIONS << "\n";
    std::cout << "Threads        : " << NUM_THREADS << "\n";
    std::cout << "Access pattern : " 
              << (useRandomAccess ? "Random" : "Sequential") << "\n\n";

    std::vector<char> large_buffer(BUFFER_SIZE, 0);
    std::atomic<size_t> total_bytes_processed{0};

    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    threads.reserve(NUM_THREADS);

    size_t chunk_size = BUFFER_SIZE / NUM_THREADS;

    for (int t = 0; t < NUM_THREADS; ++t) {
        char* chunk_start = large_buffer.data() + t * chunk_size;

        ThreadWork work {
            chunk_start,
            chunk_size,
            &total_bytes_processed,
            ITERATIONS
        };

        threads.emplace_back(memory_stress, work);
    }

    for (auto &th : threads) {
        th.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_sec = end_time - start_time;

    // throughput in MB/s
    double bytes_processed = static_cast<double>(total_bytes_processed.load());
    double bytes_per_sec = bytes_processed / elapsed_sec.count();
    double megabytes_per_sec = bytes_per_sec / (1024.0 * 1024.0);

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total bytes processed : " << bytes_processed << " bytes\n";
    std::cout << "Elapsed time          : " << elapsed_sec.count() << " s\n";
    std::cout << "Throughput            : " << megabytes_per_sec << " MB/s\n";

    return 0;
}
