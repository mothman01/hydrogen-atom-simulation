#pragma once
#include <vector>
#include <atomic>
#include <thread>
#include <functional>
#include <mutex>

// Background worker for computing volume data without blocking the render thread
class AsyncVolumeComputer {
public:
    AsyncVolumeComputer();
    ~AsyncVolumeComputer();
    
    // Start computing new volume data. If a computation is already in progress,
    // it will be cancelled and replaced.
    // fn: callable (int resolution, double halfSize, std::vector<float>& output)
    void startCompute(int resolution, double halfSize);
    
    // Set the compute function
    void setComputeFunc(std::function<void(int, double, std::vector<float>&)> fn);
    
    // Check if new data is ready. If so, pop it into output.
    // Returns true if new data was available.
    bool tryGetData(std::vector<float>& output, double& outHalfSize);
    
    // Check if computation is currently running
    bool isComputing() const { return computing_.load(); }
    
private:
    void workerLoop();
    
    std::function<void(int, double, std::vector<float>&)> computeFunc_;
    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> computing_{false};
    std::atomic<bool> dataReady_{false};
    std::atomic<bool> cancel_{false};
    
    int currentRes_ = 128;
    double currentHalfSize_ = 6.0;
    
    std::vector<float> resultData_;
    double resultHalfSize_ = 6.0;
    std::mutex resultMutex_;
};
