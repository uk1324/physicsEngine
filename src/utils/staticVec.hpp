#pragma once

#include <utils/int.hpp>
#include <utils/asserts.hpp>

template<typename T, usize CAPACITY>
class StaticVec {
public:
	StaticVec();
	~StaticVec();

	auto push(const T& value) -> void;
	auto pop() -> void;

	auto data() -> T* { return reinterpret_cast<T*>(data_); }
	auto data() const -> const T* { return reinterpret_cast<const T*>(data_); }
	auto size() -> usize { return size_; }
	auto operator[](usize i) const -> const T&;

	auto begin() -> T*;
	auto end() -> T*;
	auto cbegin() -> const T*;
	auto cend() -> const T*;

private:
	alignas(T) uint8_t data_[CAPACITY * sizeof(T)];
	usize size_;
};

template<typename T, usize CAPACITY>
StaticVec<T, CAPACITY>::StaticVec() 
	: size_{ 0 } {}

template<typename T, usize CAPACITY>
StaticVec<T, CAPACITY>::~StaticVec() {
	for (auto& v : *this) {
		v.~T();
	}
}

template<typename T, usize CAPACITY>
auto StaticVec<T, CAPACITY>::push(const T& value) -> void {
	if (size_ + 1 > CAPACITY) {
		ASSERT_NOT_REACHED();
		return;
	}
	new (&data()[size_]) T(value);
	size_++;
}

template<typename T, usize CAPACITY>
auto StaticVec<T, CAPACITY>::pop() -> void {
	if (size_ == 0) {
		ASSERT_NOT_REACHED();
		return;
	}
	data()[size_ - 1].~T();
	size_--;
}

template<typename T, usize CAPACITY>
auto StaticVec<T, CAPACITY>::operator[](usize i) const -> const T& {
	return data()[i];
}

template<typename T, usize CAPACITY>
auto StaticVec<T, CAPACITY>::begin() -> T* {
	return data();
}

template<typename T, usize CAPACITY>
auto StaticVec<T, CAPACITY>::end() -> T* {
	return data() + size_;
}

template<typename T, usize CAPACITY>
auto StaticVec<T, CAPACITY>::cbegin() -> const T* {
	return data();
}

template<typename T, usize CAPACITY>
auto StaticVec<T, CAPACITY>::cend() -> const T* {
	return data() + size_;
}
