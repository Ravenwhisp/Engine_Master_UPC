#pragma once
#include <vector>
#include <stdexcept>

template <class T>
class cvector
{
public:
    explicit cvector(size_t capacity = 100)
        : data(capacity), start(0), tail(0), count(0), capacity(capacity) {}

    void push_back(const T& value) {
        if (capacity == 0) return;

        if (count < capacity) {
            data[tail] = value;
            tail = (tail + 1) % capacity;
            count++;
        }
        else {
            data[tail] = value;
            tail = (tail + 1) % capacity;
            start = (start + 1) % capacity;
        }
    }

    void push_back(T&& value) {
        if (capacity == 0) return;

        if (count < capacity) {
            data[tail] = std::move(value);
            tail = (tail + 1) % capacity;
            count++;
        }
        else {
            data[tail] = std::move(value);
            tail = (tail + 1) % capacity;
            start = (start + 1) % capacity;
        }
    }

    size_t size() const { return count; }

    size_t get_capacity() const { return capacity; }

    bool empty() const { return count == 0; }

    bool full() const { return count == capacity; }

    void clear() {
        start = 0;
        tail = 0;
        count = 0;
    }

    T& operator[](size_t index) {
        if (index >= count) {
            throw std::out_of_range("cvector index out of range");
        }
        return data[(start + index) % capacity];
    }

    const T& operator[](size_t index) const {
        if (index >= count) {
            throw std::out_of_range("cvector index out of range");
        }
        return data[(start + index) % capacity];
    }

    T& front() {
        if (empty()) throw std::out_of_range("cvector is empty");
        return data[start];
    }

    const T& front() const {
        if (empty()) throw std::out_of_range("cvector is empty");
        return data[start];
    }

    // Get the newest element (back)
    T& back() {
        if (empty()) throw std::out_of_range("cvector is empty");
        return data[(tail + capacity - 1) % capacity];
    }

    const T& back() const {
        if (empty()) throw std::out_of_range("cvector is empty");
        return data[(tail + capacity - 1) % capacity];
    }

    std::vector<T> linearized() const {
        std::vector<T> result(count);
        if (empty()) return result;

        if (start < tail) {
            // No wrap-around, single contiguous block
            std::copy(data.begin() + start, data.begin() + tail, result.begin());
        }
        else {
            // Wrap-around, two blocks
            size_t first_part = capacity - start;
            std::copy(data.begin() + start, data.end(), result.begin());
            std::copy(data.begin(), data.begin() + tail, result.begin() + first_part);
        }
        return result;
    }

    void resize(size_t new_capacity) {
        if (new_capacity == capacity) return;

        std::vector<T> new_data(new_capacity);
        size_t elements_to_keep = std::min(count, new_capacity);

        for (size_t i = 0; i < elements_to_keep; ++i) {
            size_t src_idx = (start + count - elements_to_keep + i) % capacity;
            new_data[i] = std::move(data[src_idx]);
        }

        data = std::move(new_data);
        start = 0;
        tail = elements_to_keep % new_capacity;
        count = elements_to_keep;
        capacity = new_capacity;
    }

    class iterator {
    public:
        iterator(cvector* vec, size_t pos) : vec(vec), pos(pos) {}

        T& operator*() { return (*vec)[pos]; }
        iterator& operator++() { ++pos; return *this; }
        iterator operator++(int) { iterator temp = *this; ++pos; return temp; }
        bool operator==(const iterator& other) const { return pos == other.pos; }
        bool operator!=(const iterator& other) const { return pos != other.pos; }

    private:
        cvector* vec;
        size_t pos;
    };

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, count); }

private:
    std::vector<T> data;
    size_t start;      // Index of oldest element
    size_t tail;        // Index where next element will be inserted
    size_t count;      // Current number of elements
    size_t capacity;   // Maximum capacity
};