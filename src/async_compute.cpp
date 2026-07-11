#include "async_compute.h"
#include <thread>
#include <chrono>

AsyncVolumeComputer::AsyncVolumeComputer() {
    running_ = true;
    worker_ = std::thread(&AsyncVolumeComputer::workerLoop, this);
}

AsyncVolumeComputer::~AsyncVolumeComputer() {
    running_ = false;
    cancel_ = true;
    if (worker_.joinable()) worker_.join();
}

void AsyncVolumeComputer::setComputeFunc(std::function<void(int, double, std::vector<float>&)> fn) {
    computeFunc_ = std::move(fn);
}

void AsyncVolumeComputer::startCompute(int resolution, double halfSize) {
    cancel_ = true;
    // Small yield to let worker see the cancel
    std::this_thread::yield();
    cancel_ = false;
    currentRes_ = resolution;
    currentHalfSize_ = halfSize;
    computing_ = true;
}

void AsyncVolumeComputer::workerLoop() {
    while (running_) {
        if (computing_ && computeFunc_ && !cancel_) {
            std::vector<float> data;
            int res = currentRes_;
            double hs = currentHalfSize_;
            computeFunc_(res, hs, data);
            
            if (!cancel_) {
                std::lock_guard<std::mutex> lock(resultMutex_);
                resultData_ = std::move(data);
                resultHalfSize_ = hs;
                dataReady_ = true;
            }
            computing_ = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

bool AsyncVolumeComputer::tryGetData(std::vector<float>& output, double& outHalfSize) {
    if (!dataReady_) return false;
    std::lock_guard<std::mutex> lock(resultMutex_);
    output = std::move(resultData_);
    outHalfSize = resultHalfSize_;
    dataReady_ = false;
    return true;
}
