// Fill out your copyright notice in the Description page of Project Settings.

#include "ProceduralMeshDemos.h"
#include "AudioSink.h"
#include <algorithm>
#include <iostream>

AudioSink::AudioSink(int BitsPerSample, int nChannels, int ChunkSize)
{
	// Umm the constructor arguments aren't even used?
	if (BitsPerSample != 16)
	{
		throw "only 16 supported right now";
	}
	else
	{
		m_maxSampleVal = (1 << BitsPerSample) - 1;
	}
	m_bitDepth = BitsPerSample;
	m_nChannels = nChannels;
	m_chunkSize = ChunkSize;
}

AudioSink::~AudioSink()
{
}

bool AudioSink::Dequeue(AudioChunk& Chunk)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_queue.size() == 0)
		return false;

	Chunk = m_queue.front();
	m_queue.pop();
	return true;

	/*
	int count = std::min((int)m_queue.size(), Count);
	for (int i = 0; i < count; i++)
	{
		outChunks->push_back(m_queue.front());
		m_queue.pop();
	}
	*/
}

/*
int AudioSink::CopyData(const BYTE* Data, const int NumFramesAvailable)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	//if (NumFramesAvailable % 448 != 0 || NumFramesAvailable == 0 || Data == NULL)
	if (NumFramesAvailable % 480 != 0 || NumFramesAvailable == 0 || Data == NULL)
	{
		std::cout << "sample count wasn't 448, was: " << NumFramesAvailable << std::endl;
		return 1;
	}

	const int divisor = sizeof(long) / sizeof(short);
	short* dataStart = (short*)Data;

	// Times two because there are frames * 2 samples. Because stereo
	short* dataEnd = dataStart + NumFramesAvailable * 2;
	for (short* data = (short*)Data; data < dataEnd; data += 128)
	{
		AudioChunk* chunk = new AudioChunk();

		long long* copier = new long long[128 / divisor];

		for (int i = 0; i < (128 / divisor); i++)
		{
			copier[i] = ((long long*)data)[i];
		}

		chunk->chunk = (short*)copier;

		chunk->size = 128;

		m_queue.push(chunk);
	}

	return 0;
}
*/

int AudioSink::CopyData(const BYTE* Data, const int NumFramesAvailable)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	//AudioChunk chunk = new AudioChunk();
	AudioChunk chunk;
	if (Data == NULL)
	{
		chunk.size = 0;
		m_queue.push(chunk);
		return 0;
	}
	chunk.size = NumFramesAvailable * 2;
	chunk.chunk = new int16[chunk.size];
	std::memcpy(chunk.chunk, Data, chunk.size);
	m_queue.push(chunk);
	return 0;
}
