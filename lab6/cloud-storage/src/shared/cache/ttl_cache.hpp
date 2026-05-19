#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

template<typename K, typename V>
class TtlCache {
public:
    struct Stats {
        long long hits;
        long long misses;
        double hit_rate;  // hits / (hits + misses), или 0 если запросов не было
    };

    explicit TtlCache(std::chrono::seconds ttl) : ttl_(ttl) {}

    std::optional<V> Get(const K& key) const {
        std::shared_lock lock(mutex_);
        auto it = map_.find(key);
        if (it == map_.end() || std::chrono::steady_clock::now() > it->second.expires_at) {
            ++misses_;
            return std::nullopt;
        }
        ++hits_;
        return it->second.value;
    }

    Stats GetStats() const {
        long long h = hits_.load();
        long long m = misses_.load();
        long long total = h + m;
        return {h, m, total > 0 ? static_cast<double>(h) / total : 0.0};
    }

    void Set(const K& key, V value) {
        std::unique_lock lock(mutex_);
        map_[key] = {std::move(value), std::chrono::steady_clock::now() + ttl_};
    }

    void Invalidate(const K& key) {
        std::unique_lock lock(mutex_);
        map_.erase(key);
    }

    void InvalidateAll() {
        std::unique_lock lock(mutex_);
        map_.clear();
    }

private:
    struct Entry {
        V value;
        std::chrono::steady_clock::time_point expires_at;
    };

    mutable std::shared_mutex mutex_;
    std::unordered_map<K, Entry> map_;
    std::chrono::seconds ttl_;
    mutable std::atomic<long long> hits_{0};
    mutable std::atomic<long long> misses_{0};
};
