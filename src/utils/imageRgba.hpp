#pragma once

#include <math/vec3.hpp>
#include <math/vec2.hpp>
#include <utils/iter2d.hpp>

#include <optional>

struct PixelRgba {
	PixelRgba(u8 r, u8 g, u8 b, u8 a = 255);
	explicit PixelRgba(u8 v, u8 a = 255);
	PixelRgba(const Vec3& color);
	static auto scientificColoring(float v, float minV, float maxV) -> PixelRgba;
	static auto fromHsv(float h, float s, float v) -> PixelRgba;

	auto grayscaled() const -> PixelRgba;

	static const PixelRgba RED;
	static const PixelRgba GREEN;
	static const PixelRgba BLUE;
	static const PixelRgba BLACK;
	static const PixelRgba WHITE;

	u8 r, g, b, a;
};

class ImageRgba {
	using IndexedPixelIterator = Iter2d<PixelRgba>;
public:
	explicit ImageRgba(const char* path);
	explicit ImageRgba(Vec2T<i64> size);
	ImageRgba(const ImageRgba& other);
	ImageRgba(ImageRgba&& other) noexcept;
	auto operator=(const ImageRgba& other) -> ImageRgba&;
	auto operator=(ImageRgba&& other) noexcept -> ImageRgba&;
	~ImageRgba();

	static auto fromFile(const char* path) -> std::optional<ImageRgba>;
	auto saveToFile(const char* path) const -> void;
	auto copyAndResize(const ImageRgba& other) -> void;

	auto set(Vec2T<i64> pos, PixelRgba color) -> void;
	auto set(Vec2T<i64> pos, const Vec3& color) -> void;
	auto get(Vec2T<i64> pos) const -> PixelRgba;
	auto size() const -> const Vec2T<i64>&;
	auto data() -> PixelRgba*;
	auto dataSizeBytes() const -> usize;

	auto begin() -> PixelRgba*;
	auto end() -> PixelRgba*;
	auto cbegin() const -> const PixelRgba*;
	auto cend() const -> const PixelRgba*;

	struct IndexedPixelRange {
		ImageRgba& image;
		auto begin() -> IndexedPixelIterator;
		auto end() -> IndexedPixelIterator;
	};
	auto indexed() -> IndexedPixelRange;

	auto grayscale() -> void;

private:
	explicit ImageRgba(const char* path, bool& loadedCorrectly);

protected:
	PixelRgba* data_;
	Vec2T<i64> size_;
};