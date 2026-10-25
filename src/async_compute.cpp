#include "async_compute.h"

AsyncVolumeComputer::AsyncVolumeComputer() {
    worker_ = std::thread(&AsyncVolumeComputer::workerLoop, this);
}

AsyncVolumeComputer::~AsyncVolumeComputer() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
}

void AsyncVolumeComputer::setComputeFunc(std::function<void(int, double, std::vector<float>&)> fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    computeFunc_ = std::move(fn);
}

void AsyncVolumeComputer::startCompute(int resolution, double halfSize) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        currentRes_ = resolution;
        currentHalfSize_ = halfSize;
        ++requestId_;          // any result from an older request is discarded
        pending_ = true;
        computing_ = true;
    }
    cv_.notify_one();
}

void AsyncVolumeComputer::workerLoop() {
    std::unique_lock<std::mutex> lock(mutex_);
    for (;;) {
        cv_.wait(lock, [this] { return !running_ || pending_; });
        if (!running_) return;

        pending_ = false;
        const unsigned long long id = requestId_;
        const int res = currentRes_;
        const double hs = currentHalfSize_;
        auto fn = computeFunc_;
        lock.unlock();

        std::vector<float> data;
        if (fn) fn(res, hs, data);

        lock.lock();
        if (id == requestId_) {          // still the latest request?
            resultData_ = std::move(data);
            resultHalfSize_ = hs;
            dataReady_ = true;
        }
        if (!pending_) computing_ = false;
    }
}

bool AsyncVolumeComputer::tryGetData(std::vector<float>& output, double& outHalfSize) {
    if (!dataReady_) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    if (!dataReady_) return false;
    output = std::move(resultData_);
    outHalfSize = resultHalfSize_;
    dataReady_ = false;
    return true;
}
