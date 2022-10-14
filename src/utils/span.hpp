#pragma once

#include <iterator>
#include <vector>

template<typename T>
class Span {
public:
	Span(T* data, size_t size);
	Span(std::vector<T>& v);
	Span(const std::vector<std::remove_const_t<T>>& v);

	auto operator[](usize index) -> T&;
	auto operator[](usize index) const -> const T&;

	auto data() -> T*;
	auto data() const -> const T*;
	auto size() const -> size_t;

	auto begin() -> T*;
	auto end() -> T*;
	auto cbegin() const -> const T*;
	auto cend() const -> const T*;

	auto rbegin() -> std::reverse_iterator<T*>;
	auto rend() -> std::reverse_iterator<T*>;
	auto crbegin() const -> std::reverse_iterator<const T*>;
	auto crend() const -> std::reverse_iterator<const T*>;

private:
	T* m_data;
	size_t m_size;
};

template<typename T>
Span<T>::Span(T* data, size_t size)
	: m_data(data)
	, m_size(size) {}

template<typename T>
Span<T>::Span(std::vector<T>& v) 
	: m_data{ v.data() }
	, m_size{ v.size() } {}

template<typename T>
Span<T>::Span(const std::vector<std::remove_const_t<T>>& v)
	: m_data{ v.data() }
	, m_size{ v.size() } {}

template<typename T>
auto Span<T>::operator[](usize index) -> T& {
	ASSERT(index < m_size);
	return m_data[index];
}

template<typename T>
auto Span<T>::operator[](usize index) const -> const T& {
	return const_cast<T&>((const_cast<const Span<T>&>(*this))[index]);
}

template<typename T>
auto Span<T>::data() -> T* {
	return m_data;
}

template<typename T>
auto Span<T>::data() const -> const T* {
	return m_data;
}

template<typename T>
auto Span<T>::size() const -> size_t {
	return m_size;
}

template<typename T>
auto Span<T>::begin() -> T* {
	return m_data;
}

template<typename T>
auto Span<T>::end() -> T* {
	return m_data + m_size;
}

template<typename T>
auto Span<T>::cbegin() const -> const T* {
	return m_data;
}

template<typename T>
auto Span<T>::cend() const -> const T* {
	return m_data + m_size;
}

template<typename T>
auto Span<T>::rbegin() -> std::reverse_iterator<T*> {
	return std::reverse_iterator{ end() };
}

template<typename T>
auto Span<T>::rend() -> std::reverse_iterator<T*> {
	return std::reverse_iterator{ begin() };
}

template<typename T>
auto Span<T>::crbegin() const -> std::reverse_iterator<const T*> {
	return std::reverse_iterator{ cend() };
}

template<typename T>
auto Span<T>::crend() const -> std::reverse_iterator<const T*> {
	return std::reverse_iterator{ cbegin() };
}