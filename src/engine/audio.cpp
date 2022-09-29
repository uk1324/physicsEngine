#include <pch.hpp>
#include <engine/audio.hpp>
#include <utils/int.hpp>
#include <math/utils.hpp>
#include <winUtils.hpp>
#include <game/input.hpp>

#include <xaudio2.h>

#include <cmath>
#include <algorithm>

static ComPtr<IXAudio2> xAudio;
static IXAudio2MasteringVoice* masteringVoice;
// Can't use ComPtr with voices because they don't provide a release method. That is because destroying voices is a blocking operation.
static IXAudio2SourceVoice* sourceVoice;
static WAVEFORMATEX audioFormat;
static XAUDIO2_BUFFER buffer;

static constexpr auto SAMPLES_PER_SECOND = 44100;

auto Audio::init() -> void {
	CHECK_WIN_HRESULT(XAudio2Create(xAudio.GetAddressOf(), XAUDIO2_DEBUG_ENGINE));
	const XAUDIO2_DEBUG_CONFIGURATION debugConfig{
		.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS,
		.BreakMask = XAUDIO2_LOG_WARNINGS,
		.LogThreadID = false,
		.LogFileline = true,
		.LogFunctionName = true,
		.LogTiming = false,
	};
	xAudio->SetDebugConfiguration(&debugConfig);

	CHECK_WIN_HRESULT(xAudio->CreateMasteringVoice(&masteringVoice));
	
	static constexpr auto SAMPLES_COUNT = 2 * SAMPLES_PER_SECOND;

	audioFormat = {
		.wFormatTag = WAVE_FORMAT_PCM,
		.nChannels = 1,
		.nSamplesPerSec = SAMPLES_PER_SECOND,
		.nAvgBytesPerSec = sizeof(u16) * SAMPLES_PER_SECOND,
		.nBlockAlign = sizeof(u16),
		.wBitsPerSample = sizeof(u16) * 8,
		.cbSize = sizeof(WAVEFORMATEX),
	};
	CHECK_WIN_HRESULT(xAudio->CreateSourceVoice(&sourceVoice, &audioFormat));

	static u16 wave[SAMPLES_COUNT];
	for (auto i = 0; i < SAMPLES_COUNT; i++) {
		auto t{ static_cast<float>(i) / static_cast<float>(SAMPLES_PER_SECOND) };
		float v = sin(t * TAU<float> * 210.0f);
		/*float v = asin(sin(t * TAU<float> * 110.0f));*/
		//float v = asin(sin(t * TAU<float> * 110.0f));
		/*t += 2.0;
		float v = asin(sin(t * t * TAU<float> * 210.0f)) + 0.5;*/
		//float v = sin(fmod(t, PI<float>) * TAU<float> * 410.0f);
		/*if (v < 0.0f) v = -1.0f;
		else v = 1.0f;*/

		//float v = 0.0f;
		//for (float i = 1.0f; i < 10.0f; i++) {
		//	v += sin(t * i * TAU<float> * 110.0f * 0.5f) / i;
		//}
		//t += 0.2f;
		//float v = fmod(log10(t) * TAU<float> * 110.0f * 0.5f, 1.0f);
		//v += sin(t * TAU<float> * 110.0f);

		/*t += 1.0f;
		float v = sin(1.0 / (t * 2.0f) * TAU<float> * 410.0f);*/

		/*float v = asin(sin(t * TAU<float> * 410.0f)) * sin(2.0f * t * TAU<float> * 410.0f);*/


		const auto amplitude = 0.1f;
		v *= amplitude;
		v = std::clamp(v, -1.0f, 1.0f);
		wave[i] = static_cast<u16>(v * MAXUINT16);
	}

	buffer = {
		.Flags = 0,
		.AudioBytes = SAMPLES_COUNT * sizeof(u16),
		.pAudioData = reinterpret_cast<BYTE*>(wave),
		.PlayBegin = 0,
		//.PlayLength = 0, // Play the whole thing.
		.PlayLength = SAMPLES_COUNT,
		.LoopBegin = 0,
		.LoopLength = 0,
		.LoopCount = XAUDIO2_LOOP_INFINITE,
		.pContext = nullptr,
	};

	//CHECK_WIN_HRESULT(sourceVoice->Start());
}

auto Audio::update() -> void {
	const Keycode keys[] = { Keycode::A, Keycode::S, Keycode::D, Keycode::F, Keycode::G, Keycode::H, Keycode::J, Keycode::K };

	static float scale = 1.0;
	float s = 1.2f;
	if (Input::isKeyDown(Keycode::UP)) {
		scale *= s;
	} else if (Input::isKeyDown(Keycode::DOWN)) {
		scale /= s;
	}

	for (u32 i = 0; i < std::size(keys); i++) {
		//if (Input::isKeyDown(keys[i])) {
		//	CHECK_WIN_HRESULT(sourceVoice->SetFrequencyRatio(pow(pow(2.0f, 1.0f / 12.0f) * scale, static_cast<float>(i))));
		//	CHECK_WIN_HRESULT(sourceVoice->FlushSourceBuffers());
		//	CHECK_WIN_HRESULT(sourceVoice->SubmitSourceBuffer(&buffer));
		//	CHECK_WIN_HRESULT(sourceVoice->Start());
		//	break;
		//}
		//else if (Input::isKeyUp(keys[i])) {
		//	CHECK_WIN_HRESULT(sourceVoice->Stop());
		//	break;
		//}
	}
}
