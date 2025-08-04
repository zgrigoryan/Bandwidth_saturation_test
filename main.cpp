#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

// ---------------- Configuration ----------------
static const int    NUM_THREADS  = 8;                            // default threads
static const size_t BUFFER_SIZE  = 512ull * 1024ull * 1024ull;   // 512 MB
static const int    ITERATIONS   = 10;                           // loops over buffer
// ------------------------------------------------------------

using Clock = std::chrono::high_resolution_clock;

struct StartGate {
    std::mutex m;
    std::condition_variable cv;
    bool go = false;
    void wait() {
        std::unique_lock<std::mutex> lk(m);
        cv.wait(lk, [&]{ return go; });
    }
    void release() {
        std::lock_guard<std::mutex> lk(m);
        go = true;
        cv.notify_all();
    }
};

struct ThreadResult {
    std::uint64_t bytes_processed = 0;
    std::uint64_t checksum = 0; // prevent optimizing away
};

int main(int argc, char** argv) {
    // Parse mode
    bool random_access = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-r" || std::string(argv[i]) == "--random") {
            random_access = true;
        }
    }

    // Info banner
    std::cout << "Memory Stress Test\n"
              << "------------------\n"
              << "Buffer size    : " << BUFFER_SIZE << " bytes\n"
              << "Iterations     : " << ITERATIONS << "\n"
              << "Threads        : " << NUM_THREADS << "\n"
              << "Access pattern : " << (random_access ? "Random" : "Sequential") << "\n\n";

    // Allocate big buffer as 64-bit words (helps throughput)
    const size_t words = BUFFER_SIZE / sizeof(std::uint64_t);
    if (words == 0) {
        std::cerr << "BUFFER_SIZE too small.\n";
        return 1;
    }

    std::vector<std::uint64_t> buf(words, 0);

    // Partition work per thread
    const size_t words_per_thread = (words + NUM_THREADS - 1) / NUM_THREADS;

    // Thread coordination
    StartGate gate;
    std::vector<std::thread> threads;
    std::vector<ThreadResult> results(NUM_THREADS);

    // Work lambda
    auto worker = [&](int tid) {
        const size_t begin = std::min(words, static_cast<size_t>(tid) * words_per_thread);
        const size_t end   = std::min(words, begin + words_per_thread);
        if (begin >= end) return;

        // Simple PRNG per thread for random indices
        std::mt19937_64 rng(0xC0FFEEu ^ (static_cast<std::uint64_t>(tid) << 32));
        std::uniform_int_distribution<size_t> dist(begin, end - 1);

        // Wait for synchronized start
        gate.wait();

        std::uint64_t local_sum = 0;
        std::uint64_t bytes = 0;

        // Main loop
        for (int it = 0; it < ITERATIONS; ++it) {
            if (!random_access) {
                // Sequential pass over [begin, end)
                for (size_t i = begin; i < end; ++i) {
                    // Read
                    std::uint64_t v = buf[i];
                    local_sum += (v ^ (local_sum << 1));
                    // Write (simple mixing)
                    buf[i] = v ^ 0xA5A5A5A5A5A5A5A5ull;
                }
                bytes += (end - begin) * sizeof(std::uint64_t) * 2ull; // read + write
            } else {
                // Random accesses of equal count
                const size_t cnt = (end - begin);
                for (size_t k = 0; k < cnt; ++k) {
                    const size_t i = dist(rng);
                    std::uint64_t v = buf[i];
                    local_sum += (v + 0x9E3779B97F4A7C15ull);
                    buf[i] = v ^ 0xDEADBEEFCAFEBABEull;
                }
                bytes += cnt * sizeof(std::uint64_t) * 2ull;
            }
        }

        results[tid].bytes_processed = bytes;
        results[tid].checksum = local_sum; // make side effects observable
    };

    // Launch threads
    threads.reserve(NUM_THREADS);
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back(worker, t);
    }

    // Start timer and release gate
    auto t0 = Clock::now();
    gate.release();

    // Join
    for (auto& th : threads) th.join();
    auto t1 = Clock::now();

    // Aggregate results
    std::uint64_t total_bytes = 0;
    std::uint64_t total_checksum = 0;
    for (const auto& r : results) {
        total_bytes += r.bytes_processed;
        total_checksum ^= r.checksum; // combine so it's not optimized away
    }

    std::chrono::duration<double> sec = t1 - t0;
    const double seconds = sec.count();
    const double mb = static_cast<double>(total_bytes) / (1024.0 * 1024.0);
    const double mbps = seconds > 0 ? (mb / seconds) : 0.0;

    // Report
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total bytes processed : " << static_cast<long double>(total_bytes) << " bytes\n";
    std::cout << "Elapsed time          : " << seconds << " s\n";
    std::cout << "Throughput            : " << mbps << " MB/s\n";
    std::cout << "Checksum              : 0x" << std::hex << total_checksum << std::dec << "\n";

    return 0;
}
