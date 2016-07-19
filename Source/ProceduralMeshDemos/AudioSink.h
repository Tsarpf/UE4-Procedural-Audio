#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include "Dependencies/WindowsAudioListener/IAudioSink.h"

struct AudioChunk
{
	int16* chunk;
	int size;
};

class PROCEDURALMESHDEMOS_API AudioSink : public IAudioSink
{
public:
	bool Dequeue(AudioChunk& Chunk);
	int CopyData(const BYTE* Data, const int NumFramesAvailable) override;
	AudioSink(int BitsPerSample, int nChannels, int ChunkSize);
	~AudioSink();
private:
	std::queue<AudioChunk> m_queue;
	int m_bitDepth;
	int m_maxSampleVal;
	int m_nChannels;
	int m_chunkSize;
	std::mutex m_mutex;
};
