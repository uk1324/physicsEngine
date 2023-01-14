#include <utils/imageRgba.hpp>
#include <stb_image/stb_image.hpp>
#include <stb_image/stb_image_resize.hpp>
#include <stb_image/stb_image_write.hpp>

ImageRgba::ImageRgba(const char* path, bool& loadedCorrectly) {
	int x, y, channelCount;
	data_ = reinterpret_cast<PixelRgba*>(stbi_load(path, &x, &y, &channelCount, STBI_rgb_alpha));
	loadedCorrectly = data_ != nullptr;
	size_ = { static_cast<usize>(x), static_cast<usize>(y) };
}

static thread_local bool loadedCorrectly;

ImageRgba::ImageRgba(const char* path)
	: ImageRgba{ path, loadedCorrectly } {
	ASSERT(loadedCorrectly);
}

ImageRgba::~ImageRgba() {
	free(data_);
}

ImageRgba::ImageRgba(Vec2T<i64> size)
	: size_{ size }
	, data_{ reinterpret_cast<PixelRgba*>(malloc(4 * size.x * size.y)) } {}

ImageRgba::ImageRgba(const ImageRgba& other) 
	: size_{ other.size_ }
	, data_{ reinterpret_cast<PixelRgba*>(malloc(other.dataSizeBytes())) }{
	if (data_ == nullptr) {
		ASSERT_NOT_REACHED();
		size_ = Vec2{ 0 };
		return;
	}
	memcpy(data_, other.data_, other.dataSizeBytes());
}

ImageRgba::ImageRgba(ImageRgba&& other) noexcept 
	: size_{ other.size_ }
	, data_{ other.data_ } {
	other.data_ = nullptr;
}

auto ImageRgba::operator=(const ImageRgba& other) -> ImageRgba& {
	if (other.dataSizeBytes() > dataSizeBytes()) {
		free(data_);
		data_ = reinterpret_cast<PixelRgba*>(malloc(other.dataSizeBytes()));
		if (data_ == nullptr) {
			size_ = Vec2T{ 0 };
			ASSERT_NOT_REACHED();
			return *this;
		}
	}
	size_ = other.size_;
	memcpy(data_, other.data_, other.dataSizeBytes());
	return *this;
}

auto ImageRgba::operator=(ImageRgba&& other) noexcept -> ImageRgba& {
	size_ = other.size_;
	data_ = other.data_;
	return *this;
}

auto ImageRgba::fromFile(const char* path) -> std::optional<ImageRgba> {
	bool isLoadedCorrectly = false;
	const ImageRgba result{ path, isLoadedCorrectly };
	if (isLoadedCorrectly)
		return result;
	return std::nullopt;
}

auto ImageRgba::saveToFile(const char* path) const -> void {
	stbi_write_png(path, static_cast<int>(size_.x), static_cast<int>(size_.x), 4, data_, size_.x * 4);
}

auto ImageRgba::copyAndResize(const ImageRgba& other) -> void {
	stbir_resize_uint8(reinterpret_cast<u8*>(other.data_), static_cast<int>(other.size_.x), static_cast<int>(other.size_.y), 0, reinterpret_cast<u8*>(data_), static_cast<int>(size_.x), static_cast<int>(size_.y), 0, 4);
}

auto ImageRgba::set(Vec2T<i64> pos, PixelRgba color) -> void {
	ASSERT(pos.x < size_.x);
	ASSERT(pos.y < size_.y);
	data_[pos.y * size_.x + pos.x] = color;
}

auto ImageRgba::set(Vec2T<i64> pos, const Vec3& color) -> void {
	const Vec3T<u8> colU8{
			static_cast<u8>(std::clamp(color.x * 255.0f, 0.0f, 255.0f)),
			static_cast<u8>(std::clamp(color.y * 255.0f, 0.0f, 255.0f)),
			static_cast<u8>(std::clamp(color.z * 255.0f, 0.0f, 255.0f))
	};
	set(pos, colU8);
}

auto ImageRgba::get(Vec2T<i64> pos) const -> PixelRgba {
	ASSERT(pos.x < size_.x);
	ASSERT(pos.y < size_.y);
	const auto pixel = data_[(pos.y * size_.x + pos.x)];
	return { pixel.r, pixel.g, pixel.a };
}

auto ImageRgba::size() const -> const Vec2T<i64>& {
	return size_;
}

auto ImageRgba::data() -> PixelRgba* {
	return data_;
}

auto ImageRgba::dataSizeBytes() const -> usize {
	return 4 * size_.x * size_.y;
}

auto ImageRgba::begin() -> PixelRgba* {
	return data_;
}

auto ImageRgba::end() -> PixelRgba* {
	return data_ + size_.x * size_.y;
}

auto ImageRgba::indexed() -> ImageRgba::IndexedPixelRange {
	return IndexedPixelRange{ *this };
}

auto ImageRgba::grayscale() -> void {
	for (auto& pixel : *this) {
		pixel = pixel.grayscaled();
	}
}

PixelRgba::PixelRgba(u8 r, u8 g, u8 b, u8 a) 
	: r{ r }
	, g{ g }
	, b{ b }
	, a{ a } {}

PixelRgba::PixelRgba(u8 v, u8 a)
	: r{ v }
	, g{ v }
	, b{ v }
	, a{ a } {}

PixelRgba::PixelRgba(const Vec3& color) 
	: r{ static_cast<u8>(std::clamp(color.x * 255.0f, 0.0f, 255.0f)) }
	, g{ static_cast<u8>(std::clamp(color.y * 255.0f, 0.0f, 255.0f)) }
	, b{ static_cast<u8>(std::clamp(color.z * 255.0f, 0.0f, 255.0f)) }
	, a{ 255 } {}

auto PixelRgba::grayscaled() const -> PixelRgba {
	auto v = static_cast<u8>((static_cast<float>(r) * 0.3f + static_cast<float>(g) * 0.59f + static_cast<float>(b) * 0.11f));
	return PixelRgba{ v, a };
}

auto ImageRgba::IndexedPixelRange::begin() -> IndexedPixelIterator {
	return IndexedPixelIterator{ .pos = Vec2T{ 0 }, .data = image.data_, .rowWidth = static_cast<i64>(image.size_.x) };
}

auto ImageRgba::IndexedPixelRange::end() -> IndexedPixelIterator {
	return IndexedPixelIterator{ .pos = Vec2T{ 0 }, .data = image.data_ + image.size_.y * image.size_.x, .rowWidth = static_cast<i64>(image.size_.x) };
}
