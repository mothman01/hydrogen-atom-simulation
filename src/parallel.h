#pragma once

#include <algorithm>
#include <thread>
#include <vector>

// ---------------------------------------------------------------------------
// Minimal portable parallel-for built on std::thread.
//
// Splits [begin, end) into one contiguous chunk per hardware thread and calls
// body(i) for every index.  Used instead of OpenMP so that every platform
// (including Apple Clang, which ships without OpenMP) gets multi-core volume
// computation with no extra runtime libraries to distribute.
// ---------------------------------------------------------------------------
template <typename Body>
void parallelFor(int begin, int end, const Body& body)
{
    const int count = end - begin;
    if (count <= 0) return;

    const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    const int threads = std::min<int>(static_cast<int>(hw), count);
    if (threads <= 1) {
        for (int i = begin; i < end; ++i) body(i);
        return;
    }

    const int chunk = (count + threads - 1) / threads;
    std::vector<std::thread> pool;
    pool.reserve(static_cast<std::size_t>(threads - 1));
    for (int t = 1; t < threads; ++t) {
        const int lo = begin + t * chunk;
        const int hi = std::min(end, lo + chunk);
        if (lo >= hi) break;
        pool.emplace_back([lo, hi, &body] {
            for (int i = lo; i < hi; ++i) body(i);
        });
    }
    // The calling thread takes the first chunk.
    const int firstHi = std::min(end, begin + chunk);
    for (int i = begin; i < firstHi; ++i) body(i);

    for (auto& th : pool) th.join();
}
