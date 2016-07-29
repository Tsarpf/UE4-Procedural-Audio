
#include "ProceduralMeshDemos.h"
#include "FrequencyFinder.h"
#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>
#include "Dependencies/WindowsAudioListener/AudioListener.h"

FrequencyFinder::FrequencyFinder() 
	: m_sink(m_bitDepth, 2, 128),
	m_listener(16, WAVE_FORMAT_PCM, 4, 0)
{
	FrequencyStat stat = {
		m_max, m_min
	};
	for (int i = 0; i < 1024; i++)
	{
		m_frequencyStats.Add(stat);
	}

	m_testChunk.size = 1024;
	m_testChunk.chunk = new int16[1024];
	for (int16 i = 0; i < 1024; i++)
	{
		m_testChunk.chunk[i] = 0;
	}
}


FrequencyFinder::~FrequencyFinder()
{
	m_finished = true;
	if(m_listenerThread.joinable())
		m_listenerThread.join();
}

void FrequencyFinder::Start()
{
	m_listenerThread = std::thread(&AudioListener::RecordAudioStream, m_listener, &m_sink, std::ref(m_finished));
}

void PrintError(wchar_t* string)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, string);
}

TArray<float> FrequencyFinder::GetHeights()
{
	TArray<float> freqs;
	AudioChunk chunk;
	if (m_sink.Dequeue(chunk))
	{
		if (chunk.size == 0)
		{
			return freqs;
		}
		//CalculateFrequencySpectrum(m_testChunk.chunk, 2, m_testChunk.size, freqs);
		CalculateFrequencySpectrum(chunk.chunk, 2, chunk.size, freqs);
	}
	TArray<float> resultFloats;
	int count = freqs.Num() * 2 - 1;
	resultFloats.Reserve(count);
	for (int i = 1; i < freqs.Num() / 2; i++)
	{
		//UE_LOG(LogTemp, Log, TEXT("i: %d, Sample value %f"), i, freqs[i]);
		resultFloats.Add(freqs[i]);
	}

	return resultFloats;

	/*
	if (freqs.Num() == 0)
		return freqs;

	// size / 2 because the latter half is just a mirror of the first half
	float freqBinCount = freqs.Num() / 2;

	float maxCount = m_frequencyStats.Num();

	// The number of bins changes, so we can't map freq bins directly to the maxes.

	TArray<float> normalizedFreqs;
	//normalizedFreqs.AddUninitialized(maxCount);

	float scale = freqBinCount / maxCount;
	//float scale = maxCount / freqBinCount;
	for (float i = 0; i < maxCount; i++)
	{
		int freqIdx = (int)(scale * i);
		float newFreq = freqs[freqIdx];
		if (newFreq > m_frequencyStats[i].max)
		{
			m_frequencyStats[i].max = newFreq;
		}
		if (newFreq < m_frequencyStats[i].min)
		{
			m_frequencyStats[i].min = newFreq;
		}

		float difference = m_frequencyStats[i].max - m_frequencyStats[i].min;

		// Don't want to divide by zero
		if(difference == 0)
			normalizedFreqs.Add(0);
		else
		{
			float relativeFrequency = (newFreq - m_frequencyStats[i].min) / difference;
			normalizedFreqs.Add(relativeFrequency);
		}

	}


	return normalizedFreqs;
	//return freqs;
	*/
}

float GetFFTInValue(const int16 InSampleValue, const int16 InSampleIndex, const int16 InSampleCount)
{
	float FFTValue = InSampleValue;

	// Apply the Hann window
	FFTValue *= 0.5f * (1 - FMath::Cos(2 * PI * InSampleIndex / (InSampleCount - 1)));

	return FFTValue;
}

void FrequencyFinder::CalculateFrequencySpectrum
(
	int16* SamplePointer,
	const int32 NumChannels,
	const int32 NumAvailableSamples,
	TArray<float>& OutFrequencies
)
{
	// Clear the Array before continuing
	OutFrequencies.Empty();

	// Make sure the Number of Channels is correct
	if (NumChannels > 0 && NumChannels <= 2)
	{
		// Check if we actually have a Buffer to work with
		//if (InSoundWaveRef->CachedRealtimeFirstBuffer)
		if (SamplePointer != NULL)
		{
			// The first sample is just the StartTime * SampleRate
			//int32 FirstSample = SampleRate * InStartTime;

			// The last sample is the SampleRate times (StartTime plus the Duration)
			//int32 LastSample = SampleRate * (InStartTime + InDuration);

			// Get Maximum amount of samples in this Sound
			//const int32 SampleCount = InSoundWaveRef->RawPCMDataSize / (2 * NumChannels);
			const int32 SampleCount = NumAvailableSamples;

			// An early check if we can create a Sample window
			//FirstSample = FMath::Min(SampleCount, FirstSample);
			int32 FirstSample = 0;
			//LastSample = FMath::Min(SampleCount, LastSample);
			int32 LastSample = SampleCount;

			// Actual amount of samples we gonna read
			int32 SamplesToRead = LastSample - FirstSample;

			if (SamplesToRead < 0) {

				PrintError(TEXT("Number of SamplesToRead is < 0!"));
				return;
			}

			// Shift the window enough so that we get a PowerOfTwo. FFT works better with that
			int32 PoT = 2;

			while (SamplesToRead > PoT) {
				PoT *= 2;
			}

			// Now we have a good PowerOfTwo to work with
			SamplesToRead = PoT;

			// Create two 2-dim Arrays for complex numbers | Buffer and Output
			kiss_fft_cpx* Buffer[2] = { 0 };
			kiss_fft_cpx* Output[2] = { 0 };

			// Create 1-dim Array with one slot for SamplesToRead
			int32 Dims[1] = { SamplesToRead };

			kiss_fftnd_cfg STF = kiss_fftnd_alloc(Dims, 1, 0, nullptr, nullptr);

			//int16* SamplePtr = reinterpret_cast<int16*>(InSoundWaveRef->CachedRealtimeFirstBuffer);
			int16* SamplePtr = SamplePointer;

			// Allocate space in the Buffer and Output Arrays for all the data that FFT returns
			for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ChannelIndex++)
			{
				Buffer[ChannelIndex] = (kiss_fft_cpx*)KISS_FFT_MALLOC(sizeof(kiss_fft_cpx) * SamplesToRead);
				Output[ChannelIndex] = (kiss_fft_cpx*)KISS_FFT_MALLOC(sizeof(kiss_fft_cpx) * SamplesToRead);
			}

			// Shift our SamplePointer to the Current "FirstSample"
			SamplePtr += FirstSample * NumChannels;

			for (int32 SampleIndex = 0; SampleIndex < SamplesToRead; SampleIndex++)
			{
				for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ChannelIndex++)
				{
					// Make sure the Point is Valid and we don't go out of bounds
					if (SamplePtr != NULL && (SampleIndex + FirstSample < SampleCount))
					{
						// Use Window function to get a better result for the Data (Hann Window)
						Buffer[ChannelIndex][SampleIndex].r = GetFFTInValue(*SamplePtr, SampleIndex, SamplesToRead);
						Buffer[ChannelIndex][SampleIndex].i = 0.f;
					}
					else
					{
						// Use Window function to get a better result for the Data (Hann Window)
						Buffer[ChannelIndex][SampleIndex].r = 0.f;
						Buffer[ChannelIndex][SampleIndex].i = 0.f;
					}

					// Take the next Sample
					SamplePtr++;
				}
			}

			// Now that the Buffer is filled, use the FFT
			for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ChannelIndex++)
			{
				if (Buffer[ChannelIndex])
				{
					kiss_fftnd(STF, Buffer[ChannelIndex], Output[ChannelIndex]);
				}
			}

			OutFrequencies.AddZeroed(SamplesToRead);

			for (int32 SampleIndex = 0; SampleIndex < SamplesToRead; ++SampleIndex)
			{
				double ChannelSum = 0.0f;

				for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
				{
					if (Output[ChannelIndex])
					{
						// With this we get the actual Frequency value for the frequencies from 0hz to ~22000hz
						ChannelSum += FMath::Sqrt(FMath::Square(Output[ChannelIndex][SampleIndex].r) + FMath::Square(Output[ChannelIndex][SampleIndex].i));
					}
				}

				//if (bNormalizeOutputToDb)
				//{
					//OutFrequencies[SampleIndex] = FMath::LogX(10, ChannelSum / NumChannels) * 10;
				//}
				//else
				//{
				//	OutFrequencies[SampleIndex] = ChannelSum / NumChannels;
				//}
					//20 * log(2 * magnitude / N
					//OutFrequencies[SampleIndex] = 20 * log(2 * ChannelSum)
					//20 * log10(max(magnitude, 1e-10)); // 1e-10 = pow(10, -200/20);
					//OutFrequencies[SampleIndex] = 20 * FMath::LogX(10, std::max(ChannelSum, 1e-3));
					OutFrequencies[SampleIndex] = 20 * FMath::LogX(10, std::max(ChannelSum, 125000.0));
					//UE_LOG(LogTemp, Log, TEXT("ChannelSum: %f"), ChannelSum);
					//OutFrequencies[SampleIndex] = 20 * FMath::LogX(10, std::max(ChannelSum / ((double)NumChannels), 531072.0));
				//OutFrequencies[SampleIndex] = 20 * FMath::LogX(10, 2 * (ChannelSum / NumChannels) * SampleIndex);




					
			}

			// Make sure to free up the FFT stuff
			KISS_FFT_FREE(STF);

			for (int32 ChannelIndex = 0; ChannelIndex < NumChannels; ++ChannelIndex)
			{
				KISS_FFT_FREE(Buffer[ChannelIndex]);
				KISS_FFT_FREE(Output[ChannelIndex]);
			}
		}
		else {
			PrintError(TEXT("InSoundVisData.PCMData is a nullptr!"));
		}
	}
	else {
		PrintError(TEXT("Number of Channels is < 0!"));
	}
}

std::vector<float> FrequencyFinder::GetFrequencyMagnitudes(std::vector<AudioChunk*> chunks)
{

	for (int j = 0; j < chunks.size(); j++)
	{
		for (int i = 0; i < chunks[j]->size; i++)
		{
			float multiplier = 0.5 * (1 - cos(2 * M_PI * i / (m_bufferSize - 1)));
			short sample = chunks[j]->chunk[i];
			float fSample = (float)sample / (float)(pow(2, 16) - 1);
			int index = i + chunks[j]->size * j;
			m_fftBuffer[index] = fSample * multiplier;
		}
	}

	//fft it.
	float fftOutput[m_bufferSize];
	m_fftObject.do_fft(fftOutput, m_fftBuffer);     // x (real) --FFT---> f (complex)

													//compute magnitude of resultant vectors
	std::vector<float> mags;
	for (int i = 0; i < m_bufferSize / 2; i++)
	{
		float realPart = pow(fftOutput[i], 2);
		float imagPart = pow(fftOutput[m_bufferSize / 2 + i], 2);

		float mag = sqrt(realPart + imagPart);
		mags.push_back(mag);
	}
	//						ChannelSum += FMath::Sqrt(FMath::Square(Output[ChannelIndex][SampleIndex].r) + FMath::Square(Output[ChannelIndex][SampleIndex].i));

	return mags;
}

void FrequencyFinder::FindMinMax(float& Min, float& Max, const std::vector<float>& List)
{
	for (unsigned int i = 0; i < List.size(); i++)
	{
		if (List[i] > m_max)
		{
			m_max = List[i];
		}
		if (List[i] < m_min && List[i] != 0)
		{
			m_min = List[i];
		}
	}
	Min = m_min;
	Max = m_max;
}

void FrequencyFinder::Logarithmize(std::vector<float>& List)
{
	//OutFrequencies[SampleIndex] = FMath::LogX(10, ChannelSum / NumChannels) * 10;
	for (int i = 0; i < List.size(); i++)
	{
		List[i] = log10f(List[i] / 2.0f) * 10.0f;
	}
	/*
	float min, max;
	FindMinMax(min, max, List);


	auto newList = std::vector<float>();
	newList.push_back(List[1]);
	float multiplier = 1.5f;
	//for (unsigned int i = 2; i < List.size(); i *= multiplier)
	for (unsigned int i = 2; i < List.size(); i++) 
	{
		float sum = 0;
		float count = 0;
		for (int c = i; c < i * multiplier && c < List.size(); c++)
		{
			sum += List[c];
			count++;
		}
		float result = sum / count;
		//result *= pow(i,2); //way too steep
		result *= log2f(i);
		newList.push_back(result);
	}
	*/
	/*
	List.clear();
	for (unsigned int i = 0; i < newList.size(); i++)
	{
		List.push_back(newList[i]);
	}
	*/
}
