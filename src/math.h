#pragma once

#include <limits> // std::numeric_limits

constexpr float Pi = 3.14159265f;
constexpr float Infinity = std::numeric_limits<float>::infinity();

inline float radians(float degrees) {
	constexpr float deg_2_rad = Pi / 180.f;
	return degrees * deg_2_rad;
}

inline float degrees(float radians) {
	constexpr float rad_2_deg = 180.f / Pi;
	return radians * rad_2_deg;
}

// Boost hash combine.
template <typename T>
inline void hash_combine(std::size_t& seed, T value) {
	std::hash<T> hasher;
	seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}
