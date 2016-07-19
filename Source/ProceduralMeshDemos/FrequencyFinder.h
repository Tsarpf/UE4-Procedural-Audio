#pragma once
#include "AudioSink.h"
#include <thread>
#include "Dependencies/WindowsAudioListener/AudioListener.h"
#include "Dependencies/ffft/FFTRealFixLen.h"
#include <atomic>

struct FrequencyStat
{
	float max;
	float min;
};

class FrequencyFinder
{
public:
	FrequencyFinder();
	~FrequencyFinder();
	void Start();
	TArray<float> GetHeights();
	//std::vector<float> GetHeights(int ElapsedMilliseconds);


private:

void FrequencyFinder::CalculateFrequencySpectrum
(
	int16* SamplePointer,
	const int32 NumChannels,
	const int32 NumAvailableSamples,
	TArray<float>& OutFrequencies
);

	std::vector<float> GetFrequencyMagnitudes(std::vector<AudioChunk*> chunks);

	void FindMinMax(float & Min, float & Max, const std::vector<float>& List);

	void Logarithmize(std::vector<float>& List);

	const int m_bitDepth = 16;
	std::thread m_listenerThread;
	unsigned int m_sampleCounter = 0;
	//static const int m_bufferSize = 4096;
	//static const int m_bufferSize = 8192;
	static const int m_bufferSize = 1024;
	float m_fftBuffer[m_bufferSize];

	float m_min = 2147483648.f;
	float m_max = -2147483648.f;

	AudioListener m_listener;
	AudioSink m_sink;

	ffft::FFTRealFixLen<10> m_fftObject;
	std::atomic<bool> m_finished = false;

	TArray<FrequencyStat> m_frequencyStats;
};

