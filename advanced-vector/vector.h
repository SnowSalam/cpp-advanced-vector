#pragma once

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <utility>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    RawMemory(const RawMemory&) = delete;
    RawMemory(RawMemory&& other) noexcept {
        Swap(other);
    }

    RawMemory& operator=(const RawMemory& rhs) = delete;
    RawMemory& operator=(RawMemory&& rhs) noexcept {
        if (this != &rhs) {
            Swap(rhs);
        }
        return *this;
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept : data_(std::move(other.data_))
                                        , size_(other.size_) { other.size_ = 0; }

    iterator begin() noexcept { return data_.GetAddress(); }
    iterator end() noexcept { return data_.GetAddress() + size_; }
    const_iterator begin() const noexcept { return data_.GetAddress(); }
    const_iterator end() const noexcept { return data_.GetAddress() + size_; }
    const_iterator cbegin() const noexcept { return data_.GetAddress(); }
    const_iterator cend() const noexcept { return data_.GetAddress() + size_; }

    Vector& operator=(const Vector& other) {
        if (this != &other) {
            if (other.size_ > data_.Capacity()) {
                Vector rhs_copy(other);
                Swap(rhs_copy);
            }
            else {
                if (other.size_ < size_) {
                    for (size_t i = 0; i < other.size_; ++i) {
                        data_[i] = other.data_[i];
                    }
                    std::destroy_n(data_.GetAddress() + other.size_, size_ - other.size_);
                    size_ = other.size_;
                }
                else {
                    for (size_t i = 0; i < size_; ++i) {
                        data_[i] = other.data_[i];
                    }
                    std::uninitialized_copy_n(other.data_.GetAddress() + size_
                        , other.size_ - size_
                        , data_.GetAddress() + size_);
                    size_ = other.size_;
                }
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& other) noexcept {
        if (this != &other) {
            data_.Swap(other.data_);
            std::swap(size_, other.size_);
        }
        return *this;
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void PushBack(const T& value) {
        if (data_.Capacity() == 0) Reserve(1);
        if (size_ == data_.Capacity()) {
            RawMemory<T> new_data(data_.Capacity() * 2);
            new (new_data + size_) T(value);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
            ++size_;
        }
        else {
            new (data_.GetAddress() + size_) T(value);
            ++size_;
        }
    }
    
    void PushBack(T&& value) {
        if (data_.Capacity() == 0) Reserve(1);
        if (size_ == data_.Capacity()) {
            RawMemory<T> new_data(data_.Capacity() * 2);
            new (new_data + size_) T(std::move(value));
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
            ++size_;
        }
        else {
            new (data_.GetAddress() + size_) T(std::move(value));
            ++size_;
        }
    }
    
    void PopBack() noexcept {
        assert(size_ > 0);
        data_[--size_].~T();
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    void Resize(size_t new_size) {
        if (new_size > size_) {
            if (new_size > Capacity()) {
                Reserve(new_size);
            }
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, new_size - size_);
            size_ = new_size;
        }
        else if (new_size < size_) {
            std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
            size_ = new_size;
        }
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    iterator Insert(const_iterator pos, const T& value) {
        if (size_ != 0) {
            if (
                std::addressof(value) >= std::addressof(data_[0])
                && std::addressof(value) <= std::addressof(data_[size_ - 1])) {
                return Emplace(pos, T(value));
            }
        }
        return Emplace(pos, value);


        //return Emplace(pos, value);
    }
    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::forward<T>(value));
    }

    iterator Erase(const_iterator pos) {
        assert(pos >= begin() && pos < end());
        size_t index = pos - cbegin();
        std::move(begin() + index + 1, end(), begin() + index);
        (begin() + size_ - 1)->~T();
        --size_;
        return &data_[index];
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        if (size_ == data_.Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : data_.Capacity() * 2);
            size_t index = pos - cbegin();
            new (new_data + index) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), index, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), index, new_data.GetAddress());
            }

            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress() + index, size_ - index, new_data.GetAddress() + index + 1);
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress() + index, size_ - index, new_data.GetAddress() + index + 1);
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
            ++size_;
            return &data_[index];
        }
        else {
            /*
            size_t index = pos - cbegin();
            if constexpr (sizeof...(args) > 1) {
                T temp(std::forward<Args>(args)...);
                std::move_backward(data_.GetAddress() + index, end(), end());
                data_[index] = std::move(temp);
            }
            else {
                std::move_backward(data_.GetAddress() + index, end(), end());
                new (data_.GetAddress() + index) T(std::forward<Args>(args)...);
            }
            */
            /*
            size_t index = pos - cbegin();
            T* temp = new T(std::forward<Args>(args)...);
            std::uninitialized_move_n(data_.GetAddress() + size_ - 1, 1, data_.GetAddress() + size_);
            std::move_backward(data_.GetAddress() + index, end() - 1, end());
            new (data_.GetAddress() + index) T(std::move(*temp));
            delete temp;
            */
            size_t index = pos - cbegin();
            T* temp = new T(std::forward<Args>(args)...);
            std::move_backward(data_.GetAddress() + size_ - 1, data_.GetAddress() + size_, data_.GetAddress() + size_);
            std::move_backward(data_.GetAddress() + index, end() - 1, end());
            new (data_.GetAddress() + index) T(std::move(*temp));
            delete temp;

            ++size_;
            return data_.GetAddress() + index;
        }
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (data_.Capacity() == 0) Reserve(1);
        if (size_ == data_.Capacity()) {
            RawMemory<T> new_data(data_.Capacity() * 2);
            new (new_data + size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
            ++size_;
        }
        else {
            new (data_.GetAddress() + size_) T(std::forward<Args>(args)...);
            ++size_;
        }

        return data_[size_ - 1];
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};