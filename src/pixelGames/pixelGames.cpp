#include <pixelGames/pixelGames.hpp>
#include <engine/window.hpp>
#include <engine/time.hpp>
#include <math/utils.hpp>

#include <imgui/imgui.h>

#include <complex>

#include <stb_image/stb_image.hpp>


PixelGames::PixelGames(Gfx& gfx)
	: texture{ gfx, Vec2T<i32>{ 64, 64 } } {

	int width, height;
	int channels;
	image = stbi_load("assets/test.png", &width, &height, &channels, STBI_rgb_alpha);
	ASSERT(image);
	ASSERT(width == 100 && height == 100);
}


using namespace ImGui;

#include <valarray>

void fft_child(const std::complex<double>* input, std::complex<double>* target, int n, int s) {
	static constexpr auto pi = 3.1415926535897932384626433832795028;

	if (n == 1) {
		target[0] = input[0];
		return;
	} else if (n / 2 == 1) {
		target[0] = input[0];
		target[n / 2] = input[s];
	} else {
		fft_child(input, target, n / 2, 2 * s);
		fft_child(input + s, target + n / 2, n / 2, 2 * s);
	}

	for (int k = 0; k < n / 2; k++) {
		auto t = target[k];

		auto angle = -2 * pi * k / (double)n;
		auto cos_angle = ::cos(angle);
		auto sin_angle = ::sin(angle);

		auto x = target[k + n / 2];

		const auto s1 = cos_angle * x.real() - sin_angle * x.imag();
		const auto s2 = cos_angle * x.imag() + sin_angle * x.real();

		target[k] = { t.real() + s1, t.imag() + s2 };

		target[k + n / 2] = { t.real() - s1, t.imag() - s2 };

		/*target[k].real() = t.real() + s1;
		target[k].imag() = t.imag() + s2;

		target[k + n / 2].r = t.r - s1;
		target[k + n / 2].i = t.i - s2;*/
	}
}

void fft(const std::complex<double>* input, std::complex<double>* target, int n) {
	fft_child(input, target, n, 1);
}

void fft_inverse(const std::complex<double>* input, std::complex<double>* target, int n) {
	fft_child(input, target, n, 1);

	auto inv_n = 1 / (double)n;

	// Apply scale and reverse direction
	for (int i = 0; i < n; i++) {
		target[i] = target[i] * inv_n;
	}

	for (int i = 1; i < n / 2; i++) {
		auto temp = target[i];
		target[i] = target[n - i];
		target[i] = target[n - i];

		target[n - i] = temp;
		target[n - i] = temp;
	}
}



typedef std::complex<double> Complex;
typedef std::valarray<Complex> CArray;

// Size must be a power of 2
void fft(CArray& x, bool revere = true)
{
	const size_t N = x.size();
	if (N <= 1) return;

	// divide
	CArray even = x[std::slice(0, N / 2, 2)];
	CArray  odd = x[std::slice(1, N / 2, 2)];

	// conquer
	fft(even);
	fft(odd);

	// combine
	using namespace std::complex_literals;
	for (size_t k = 0; k < N / 2; ++k)
	{
		//Complex t = std::polar(1.0, (revere ? 1.0 : -1.0) * -2 * PI<double> * k / N) * odd[k];
		Complex t = std::exp((revere ? 1.0 : -1.0) * -2 * PI<double> / N * k * 1i) * odd[k];
		x[k] = even[k] + t;
		x[k + N / 2] = even[k] - t;
	}
}

void ffta(CArray& x, bool revere = true)
{
	const size_t N = x.size();
	auto a = x;

	using namespace std::complex_literals;
	//std::vector<std::complex<float>> input = { 1.0f, 2.0f - 1if, -1if, -1.0f + 2if };
	for (usize i = 0; i < x.size(); i++) {
		std::complex<float> value = 0.0f;
		for (usize j = 0; j < x.size(); j++) {
			auto input = x[j];
			//input -= 128;
			// Could precomute the powers e^(tau * i / size * _).
			value += input * std::exp((revere ? -1.0 : 1.0) * -TAU<double> * 1i * static_cast<double>(i) * static_cast<double>(j) / static_cast<double>(N));
		}
		a[i] = value;
	}

	x = a;
}


// inverse fft (in-place)
void ifft(CArray& x)
{
	// conjugate the complex numbers
	x = x.apply(std::conj);

	// forward fft
	fft(x, true);

	// conjugate the complex numbers again
	x = x.apply(std::conj);

	// scale the numbers
	x /= x.size();
}


auto PixelGames::update(Gfx& gfx, Renderer& renderer) -> void {

	Begin("game");
	const auto old = doFourierTransform;
	Checkbox("fourier transform", &doFourierTransform);
	const auto doFourierTransformChanged = old != doFourierTransform || elapsed == 0.0f;

	auto get = [](void* data, int i) -> float {
		return std::abs((*reinterpret_cast<decltype(result)*>(data))[i]);
	};
	ImGui::PlotLines("test", static_cast<float(*)(void*, int)>(get), &result, result.size());

	End();

	elapsed += Time::deltaTime();
	ImGui::ShowDemoWindow();

	// https://proofwiki.org/wiki/Fourier%27s_Theorem


	/*11:20 It might be important to note that you can't generally overlay multiple FFTs like that and have the frequencies align nicely. 
		This only works if your input signals are all the same length(and sampling rate) because the frequency domain spacing is inversely proportional to the length of the time series.*/

	// If odd could store a separate variable that stores the odd number and set it to 0 if there element count is even.
	if (doFourierTransformChanged) {
		if (doFourierTransform) {
			// The imaginary part of the result is the sine wave and the real is the cosine of the frequency. The length is the aplitude of that frequency. The frequency of 1 is the whole length of the image, 2 is half and so on.
			// The length is the aplitude of the frequency and the angle is the offset of the input to the sinusoidal function. For example e^(i * offset * x) first offset by offset then rotate by x.
			// Check if the thing above is correct.
			// When undoing fourier transform the multiplication by the result[i] first rotates by a given angle and then scales.

			// When a function is perodic the integral only needs to be taken over the period. A set of data could be interpreted as one period of a periodic function.

			// TODO: What are negative frequencies. This function is decrete so there are no nagative frequencies
			// https://www.youtube.com/watch?v=6dW6VYXp9HM

			// The inverse of a fourier transform is the fourier transform. The fourier transform returns how much a given frequency is present in the signal so the inverse it to sum up the complex sinusoids of the given frequencies.
			// https://www.youtube.com/watch?v=a8FTr2qMutA
			// https://www.youtube.com/watch?v=Iuv6hY6zsd0
			// https://www.youtube.com/watch?v=cztocbHiiqQ
			// https://www.youtube.com/watch?v=GzbKb59my3U
			// And other videos from around that time.

			// The first term is the zero frequency which is the offset the exponent multiplies out to 0 so the e^exponent = 1. The average value over the whole function.

			//result.clear();
			//using namespace std::complex_literals;
			////std::vector<std::complex<float>> input = { 1.0f, 2.0f - 1if, -1if, -1.0f + 2if };
			//for (usize _ = 0; _ < texture.size().x; _++) {
			//	std::complex<float> value = 0.0f;
			//	for (usize i = 0; i < texture.size().x; i++) {
			//		auto input = static_cast<float>(texture.get(Vec2T{ static_cast<i32>(i), 2 }).x);
			//		input -= 128;
			//		// Could precomute the powers e^(tau * i / size * _).
			//		value += input * std::exp(-TAU<float> * 1if* static_cast<float>(i)* static_cast<float>(_) / static_cast<float>(texture.size().x));
			//	}
			//	result.push_back(value);
			//}

			//float min = std::numeric_limits<float>::infinity(), max = -std::numeric_limits<float>::infinity();
			//for (i32 x = 0; x < texture.size().x; x++) {
			//	const auto value = std::abs(result[x]);
			//	min = std::min(min, value);
			//	max = std::max(max, value);
			//}

			//// Show the transform
			////for (i32 x = 0; x < texture.size().x; x++) {
			////	auto value = std::abs(result[x]);
			////	value = (static_cast<float>(value) + min) / (min + max);
			////	texture.set(Vec2T{ x, 3 }, Vec3T<u8>{ static_cast<u8>(value * 255.0f) });
			////}

			//// Inverse the transform
			//for (usize _ = 0; _ < texture.size().x; _++) {
			//	std::complex<float> value = 0.0f;
			//	for (usize i = 0; i < texture.size().x; i++) {
			//		value += result[i] * std::exp(TAU<float> * 1if * static_cast<float>(i) * static_cast<float>(_) / static_cast<float>(texture.size().x));
			//		/*value += result[i].real() * cos(TAU<float> * static_cast<float>(i)* static_cast<float>(_) / static_cast<float>(texture.size().x));*/
			//	}
			//	// value.imag() is only 0 at the end of the loop.
			//	value /= texture.size().x;
			//	value += 128;
			//	texture.set(Vec2T{ static_cast<i32>(_), 2 }, Vec3T<u8>{ static_cast<u8>(std::abs(value)) });
			//}

			//for (i32 iY = 0; iY < texture.size().y; iY++) {
			//	std::vector<std::complex<double>> data(texture.size().x);
			//	auto output = data;
			//	for (i32 iX = 0; iX < texture.size().x; iX++) {
			//		data[iX] = texture.get({ iX, iY }).x;
			//	}

			//	fft(data.data(), output.data(), data.size());
			//	fft_inverse(output.data(), data.data(), data.size());

			//	for (i32 iX = 0; iX < texture.size().x; iX++) {
			//		texture.set({ iX, iY }, Vec3T<u8>{ static_cast<u8>(std::abs(output[iX])) });
			//	}
			//}

			for (i32 iY = 0; iY < texture.size().y; iY++) {
				CArray data(texture.size().x);
				for (i32 iX = 0; iX < texture.size().x; iX++) {
					/*data[iX] = texture.get({ iX, iY }).x / 255.0f - 0.5f;*/
					data[iX] = texture.get({ iX, iY }).x;
				}
				
				fft(data);

				//data[1] = 0.0f;
				//data[2] = 0.0f;
				//data[3] = 0.0f;
				//data[4] = 0.0f;

				ifft(data);
				for (i32 iX = 0; iX < texture.size().x; iX++) {
					texture.set({ iX, iY }, Vec3T<u8>{ static_cast<u8>(std::abs(data[iX])) });
					//texture.set({ iX, iY }, Vec3T<u8>{ static_cast<u8>(data[iX].real()) });
				}
			}
			

		} else {
			for (i32 iY = 0; iY < texture.size().y; iY++) {
				for (i32 iX = 0; iX < texture.size().x; iX++) {
					//auto pos = (Vec2{ static_cast<float>(iX), static_cast<float>(iY) } / Vec2{ texture.size() } - Vec2{ 0.5f }) * 2.0f;
					//pos.x *= 10.0f;

					//float value;

					//// For real valued functions the complex part has to cancel so there should be a way to ignore it when storing the data. Maybe cos * |c|
					//auto f = [](float x, float y) -> float {
					//	/*return (sin(x * 2.0f * PI<float>) + 1.0f) / 2.0f;*/
					//	//return sin(x);
					//	//return sin(x) / x;
					//	/*return sin(x) + sin(3 * x);*/
					//	return sin(x) + sin(y);
					//	/*if (abs(x) < 1.0) {
					//		return 1.0f;
					//	}
					//	return -1.0f;*/
					//};

					//value = f(pos.x, pos.y);
					//value = (value + 1.0f) / 2.0f;
					//Vec3 col = { value };
					//const Vec3T<u8> colU8{
					//	static_cast<u8>(std::clamp(col.x * 255.0f, 0.0f, 255.0f)),
					//	static_cast<u8>(std::clamp(col.y * 255.0f, 0.0f, 255.0f)),
					//	static_cast<u8>(std::clamp(col.z * 255.0f, 0.0f, 255.0f))
					//};
					//texture.set({ iX, iY }, colU8);

					u8 r = image[(iY * 100 + iX) * 4 + 0];
					u8 g = image[(iY * 100 + iX) * 4 + 1];
					u8 b = image[(iY * 100 + iX) * 4 + 2];
					u8 v = (u8)(((float)r + float(g) + float(b)) / 3.0f);

					texture.set({ iX, iY }, Vec3T<u8>{ v });
				}
			}
		}
		
	}

	Camera camera;
	camera.aspectRatio = Window::aspectRatio();

	renderer.drawDynamicTexture(Vec2{ 0.0f }, camera.aspectRatio / 3.0f, texture);

	renderer.update(gfx, camera, Window::size(), false);
}
