#pragma once

#include <utils/asserts.hpp>

#include <vector>

template<typename T>
auto swapAndPop(std::vector<T> v, typename std::vector<T>::iterator it) -> typename std::vector<T>::iterator {
	if (it >= v.end()) {
		ASSERT_NOT_REACHED();
		return it;
	}
	if (it != v.end() - 1) {
		// Pointlessly copies the other object into the last place. Can't optimize this without access to the implementation of std::vector or making a class UnorderedArray.
		std::swap(v.end() - 1, it);
	}
	v.pop_back();
	return it;
}