#pragma once

#include <algorithm>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

class RateLimiter {
public:
    struct Result {
        bool allowed;
        int limit;
        int remaining;
        long long reset_epoch_sec;
    };

    // rate_per_min: sustained rate; burst: max tokens (= initial bucket size)
    RateLimiter(int rate_per_min, int burst)
        : rate_per_sec_(static_cast<double>(rate_per_min) / 60.0),
          burst_(burst) {}

    Result Check(const std::string& key) {
        std::lock_guard lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        auto& b = buckets_[key];

        if (b.last_refill == std::chrono::steady_clock::time_point{}) {
            b.tokens = static_cast<double>(burst_);
            b.last_refill = now;
        } else {
            double elapsed = std::chrono::duration<double>(now - b.last_refill).count();
            b.tokens = std::min<double>(burst_, b.tokens + elapsed * rate_per_sec_);
            b.last_refill = now;
        }

        long long now_epoch = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        long long secs_to_refill = (rate_per_sec_ > 0.0)
            ? static_cast<long long>((burst_ - b.tokens) / rate_per_sec_) + 1
            : 60LL;
        long long reset = now_epoch + secs_to_refill;

        if (b.tokens >= 1.0) {
            b.tokens -= 1.0;
            return {true, burst_, static_cast<int>(b.tokens), reset};
        }
        return {false, burst_, 0, reset};
    }

private:
    struct BucketState {
        double tokens{0.0};
        std::chrono::steady_clock::time_point last_refill{};
    };

    double rate_per_sec_;
    int burst_;
    std::mutex mutex_;
    std::unordered_map<std::string, BucketState> buckets_;
};
