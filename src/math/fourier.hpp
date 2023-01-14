#pragma once

#include <vector>
#include <complex>

// Could make the functions more generating and let them take an iterator maybe even to things that are not complex numbers, but are convertible to them.
auto dft(std::vector<std::complex<double>>& inOut) -> void;
auto inverseDft(std::vector<std::complex<double>>& inOut) -> void;

auto fft(std::vector<std::complex<double>>& inOut) -> void;
auto inverseFft(std::vector<std::complex<double>>& inOut) -> void;