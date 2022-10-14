#include <utils/random.hpp>
#include <random>

thread_local std::default_random_engine randomEngine;

auto randomInRange(float min, float max) -> float {
	return std::uniform_real_distribution(min, max)(randomEngine);
}
