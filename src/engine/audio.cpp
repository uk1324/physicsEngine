#include <pch.hpp>
#include <engine/audio.hpp>
#include <engine/time.hpp>
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
// TODO: Destroy voices
static IXAudio2SourceVoice* sourceVoice[8];
static WAVEFORMATEX audioFormat;
static XAUDIO2_BUFFER buffer;

static constexpr auto SAMPLES_PER_SECOND = 44100;

//static auto hz()

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
		.nAvgBytesPerSec = sizeof(i16) * SAMPLES_PER_SECOND,
		.nBlockAlign = sizeof(i16),
		.wBitsPerSample = sizeof(i16) * 8,
		.cbSize = sizeof(WAVEFORMATEX),
	};

	static i16 wave[SAMPLES_COUNT];
	for (auto i = 0; i < SAMPLES_COUNT; i++) {
		if (i == SAMPLES_COUNT - 1) {
			putc('a', stdout);
		}

		auto t{ static_cast<float>(i) / static_cast<float>(SAMPLES_PER_SECOND) };
		//float v{ sin(t * TAU<float> * 210.0f) };
		/*float v = asin(sin(t * TAU<float> * 110.0f));*/
		//float v = asin(sin(t * TAU<float> * 110.0f));
		/*t -= 1.0;
		float v = asin(sin(t * t * TAU<float> * 210.0f)) + 0.5;*/
		/*float v = sin(fmod(t, PI<float>) * TAU<float> * 410.0f);
		if (v < 0.0f) v = -1.0f;
		else v = 1.0f;*/

		/*float v = asin(sin(t * TAU<float> * 210.0f));
		if (v < 0.0f) v = -1.0f;
		else v = 1.0f;*/

		float v = 0.0f;
		for (float i = 1.0f; i < 10.0f; i++) {
			v += sin(t * i * TAU<float> * 110.0f * 0.5f) / i;
		}
		v += sin(t * TAU<float> * 210.0f);


		//t += 0.2f;
		/*float v = fmod(log10(t) * TAU<float> * 110.0f * 0.5f, 1.0f);
		v += sin(t * TAU<float> * 110.0f);*/

		/*t += 1.0f;
		float v = sin(1.0f / (t * 2.0f) * TAU<float> * 410.0f);*/

		//float v = asin(sin(t * TAU<float> * 410.0f)) * sin(2.0f * t * TAU<float> * 410.0f);


		const auto amplitude = 0.1f;
		v *= amplitude;
		v = std::clamp(v, -1.0f, 1.0f);
		wave[i] = static_cast<i16>(v * MAXUINT16);
	}

	buffer = {
		.Flags = 0,
		.AudioBytes = SAMPLES_COUNT * sizeof(i16),
		.pAudioData = reinterpret_cast<BYTE*>(wave),
		.PlayBegin = 0,
		.PlayLength = 0, // Play the whole thing.
		//.PlayLength = SAMPLES_COUNT,
		.LoopBegin = 0,
		.LoopLength = 0,
		.LoopCount = XAUDIO2_LOOP_INFINITE,
		.pContext = nullptr,
	};

	for (i32 i = 0; i < std::size(sourceVoice); i++) {
		CHECK_WIN_HRESULT(xAudio->CreateSourceVoice(&sourceVoice[i], &audioFormat));
		CHECK_WIN_HRESULT(sourceVoice[i]->FlushSourceBuffers());
		CHECK_WIN_HRESULT(sourceVoice[i]->SubmitSourceBuffer(&buffer));
		CHECK_WIN_HRESULT(sourceVoice[i]->SetVolume(0.0f));
		CHECK_WIN_HRESULT(sourceVoice[i]->Start());
	}
}

struct AdsrEnvelope {
	float timeElapsed;
	bool isHeld;

	float attackAmplitude;
	float attackLength;

	float decayLength;

	float sustainAmplitude;

	float releaseStart;
	float releaseLength;

	AdsrEnvelope() {
		timeElapsed = 0.0f;
		isHeld = false;
		attackAmplitude = 0.6f;
		attackLength = 0.1f;
		decayLength = 0.2f;
		sustainAmplitude = 0.4f;
		releaseStart = -1000.0f;
		releaseLength = 0.1f;
	}

	auto update(float deltaTime) -> void {
		timeElapsed += deltaTime;
	}

	auto getAplitude() -> float {
		float amplitude;
		if (isHeld) {
			if (timeElapsed < attackLength) {
				amplitude = lerp(0.0f, attackAmplitude, timeElapsed / attackLength);
			}
			else if (timeElapsed < attackLength + decayLength) {
				amplitude = lerp(attackAmplitude, sustainAmplitude, (timeElapsed - attackLength) / decayLength);
			}
			else {
				amplitude = sustainAmplitude;
			}
		} else {
			// Could interpolate from the amplitude before release instead of interpolating from sustainAmplitude.
			amplitude = lerp(sustainAmplitude, 0.0f, (timeElapsed - releaseStart) / releaseLength), 0.0f, 1.0f;
		}
		return std::clamp(amplitude, 0.0f, 1.0f);
	}
};

static AdsrEnvelope enveolpe[8];

#include <utils/io.hpp>

auto Audio::update() -> void {
	const Keycode keys[] = { Keycode::A, Keycode::S, Keycode::D, Keycode::F, Keycode::G, Keycode::H, Keycode::J, Keycode::K };

	static float scale = 1.0;
	float s = 1.2f;
	if (Input::isKeyDown(Keycode::UP)) {
		scale *= s;
	} else if (Input::isKeyDown(Keycode::DOWN)) {
		scale /= s;
	}

	for (i32 i = 0; i < std::size(enveolpe); i++) {
		enveolpe[i].update(Time::deltaTime());
		CHECK_WIN_HRESULT(sourceVoice[i]->SetVolume(enveolpe[i].getAplitude() * 8.0f));
	}

	for (u32 i = 0; i < std::size(keys); i++) {
		if (Input::isKeyDown(keys[i])) {
			CHECK_WIN_HRESULT(sourceVoice[i]->SetFrequencyRatio(pow(pow(2.0f, 1.0f / 12.0f) * scale, static_cast<float>(i))));
			enveolpe[i].isHeld = true;
			enveolpe[i].timeElapsed = 0.0f;
			break;
		} else if (Input::isKeyUp(keys[i])) {
			enveolpe[i].isHeld = false;
			enveolpe[i].releaseStart = enveolpe[i].timeElapsed;
			break;
		}
	}
}
