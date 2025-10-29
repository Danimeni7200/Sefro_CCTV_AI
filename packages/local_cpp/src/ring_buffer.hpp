#pragma once

#include <atomic>
#include <memory>
#include <chrono>
#include <thread>
#include "frame.hpp"

template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity) 
        : capacity_(capacity), 
          buffer_(std::make_unique<T[]>(capacity)),
          head_(0), 
          tail_(0) {}
    
    ~RingBuffer() = default;
    
    // Non-blocking push
    bool try_push(T&& item) {
        size_t current_head = head_.load(std::memory_order_relaxed);
        size_t next_head = (current_head + 1) % capacity_;
        
        if (next_head == tail_.load(std::memory_order_acquire)) {
            return false; // Buffer full
        }
        
        buffer_[current_head] = std::move(item);
        head_.store(next_head, std::memory_order_release);
        return true;
    }
    
    // Non-blocking pop
    bool try_pop(T& item) {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        
        if (current_tail == head_.load(std::memory_order_acquire)) {
            return false; // Buffer empty
        }
        
        item = std::move(buffer_[current_tail]);
        tail_.store((current_tail + 1) % capacity_, std::memory_order_release);
        return true;
    }
    
    // Blocking pop with timeout
    bool pop(T& item, std::chrono::milliseconds timeout) {
        auto start = std::chrono::steady_clock::now();
        
        while (true) {
            if (try_pop(item)) {
                return true;
            }
            
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= timeout) {
                return false;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    // Get current size (approximate)
    size_t size() const {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_acquire);
        return (head - tail + capacity_) % capacity_;
    }
    
    // Check if empty
    bool empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }
    
    // Check if full
    bool full() const {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_acquire);
        return (head + 1) % capacity_ == tail;
    }
    
    // Drop oldest item (for backpressure)
    bool drop_oldest() {
        if (empty()) return false;
        
        T dummy;
        return try_pop(dummy);
    }
    
    // Clear buffer
    void clear() {
        T dummy;
        while (try_pop(dummy)) {
            // Keep popping until empty
        }
    }
    
    size_t capacity() const { return capacity_; }

private:
    const size_t capacity_;
    std::unique_ptr<T[]> buffer_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
};




#include <atomic>
#include <memory>
#include <chrono>
#include <thread>
#include "frame.hpp"

template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity) 
        : capacity_(capacity), 
          buffer_(std::make_unique<T[]>(capacity)),
          head_(0), 
          tail_(0) {}
    
    ~RingBuffer() = default;
    
    // Non-blocking push
    bool try_push(T&& item) {
        size_t current_head = head_.load(std::memory_order_relaxed);
        size_t next_head = (current_head + 1) % capacity_;
        
        if (next_head == tail_.load(std::memory_order_acquire)) {
            return false; // Buffer full
        }
        
        buffer_[current_head] = std::move(item);
        head_.store(next_head, std::memory_order_release);
        return true;
    }
    
    // Non-blocking pop
    bool try_pop(T& item) {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        
        if (current_tail == head_.load(std::memory_order_acquire)) {
            return false; // Buffer empty
        }
        
        item = std::move(buffer_[current_tail]);
        tail_.store((current_tail + 1) % capacity_, std::memory_order_release);
        return true;
    }
    
    // Blocking pop with timeout
    bool pop(T& item, std::chrono::milliseconds timeout) {
        auto start = std::chrono::steady_clock::now();
        
        while (true) {
            if (try_pop(item)) {
                return true;
            }
            
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (elapsed >= timeout) {
                return false;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    // Get current size (approximate)
    size_t size() const {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_acquire);
        return (head - tail + capacity_) % capacity_;
    }
    
    // Check if empty
    bool empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }
    
    // Check if full
    bool full() const {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_acquire);
        return (head + 1) % capacity_ == tail;
    }
    
    // Drop oldest item (for backpressure)
    bool drop_oldest() {
        if (empty()) return false;
        
        T dummy;
        return try_pop(dummy);
    }
    
    // Clear buffer
    void clear() {
        T dummy;
        while (try_pop(dummy)) {
            // Keep popping until empty
        }
    }
    
    size_t capacity() const { return capacity_; }

private:
    const size_t capacity_;
    std::unique_ptr<T[]> buffer_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
};


