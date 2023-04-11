#pragma once

#include <math/vec2.hpp>

template<typename T>
class Array2d {
public:
	Array2d();
	Array2d(Vec2T<i64> size);
	~Array2d();

	Array2d(Array2d&& other) noexcept;
	auto operator=(Array2d&& other) noexcept -> Array2d&;

	// using operator() for multidimensional indexing is better that using operator[], because it allows any way of structuring data. When using operator[] if you want the first index to be the x and the second the y then the data has to be laid you this way(unless you make a special type). It also may be simpler to write.
	// The character count is the same.
	// [x][y]
	// (x, y)
	auto operator()(i64 x, i64 y) -> T&;
	auto operator()(i64 x, i64 y) const -> const T&;
	auto at(Vec2T<i64> pos) -> T&;
	auto at(Vec2T<i64> pos) const -> const T&;

private:
	Vec2T<i64> size_;
	T* data_;
private:
	auto size() const -> Vec2T<i64> { return size_; }
	auto data() const -> T* { return data_; }
};

template<typename T>
Array2d<T>::Array2d() 
	: data_{ nullptr }
	, size_{ 0, 0 } {}

template<typename T>
Array2d<T>::Array2d(Vec2T<i64> size) 
	: data_{ reinterpret_cast<T*>(operator new(size.x * size.y * sizeof(T))) }
	, size_{ size } {}

template<typename T>
Array2d<T>::~Array2d() {
	operator delete(data_);
}

template<typename T>
Array2d<T>::Array2d(Array2d&& other) noexcept 
	: data_{ other.data_ }
	, size_{ other.size_ } {
	other.data_ = nullptr;
}

template<typename T>
auto Array2d<T>::operator=(Array2d&& other) noexcept -> Array2d& {
	data_ = other.data_;
	size_ = other.size_;
	other.data_ = nullptr;
}

template<typename T>
auto Array2d<T>::operator()(i64 x, i64 y) -> T& {
	return *(data_ + y * size_.x + y);
}

template<typename T>
auto Array2d<T>::operator()(i64 x, i64 y) const -> const T& {
	return const_cast<Array2d*>(this)->operator()(x, y);
}

template<typename T>
auto Array2d<T>::at(Vec2T<i64> pos) -> T& {
	return operator()(pos.x, pos.y);
}

template<typename T>
auto Array2d<T>::at(Vec2T<i64> pos) const -> const T& {
	return operator()(pos.x, pos.y);
}
