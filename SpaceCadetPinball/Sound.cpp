#include "pch.h"
#include "Sound.h"


#include "pb.h"

int Sound::num_channels;
unsigned int Sound::enabled_flag = -1;
const int Sound::initFlags = MIX_INIT_MID;


int Sound::Init(int voices)
{
	int channelCount = voices;
	if (voices > 8)
		channelCount = 8;
	num_channels = channelCount;
	


	int mixerInitReturnVal = Mix_Init(initFlags);
	if(mixerInitReturnVal != initFlags)
	{
		fprintf(stderr, "ERROR COULD NOT INIT MIDI SUPPORT WITH SDL2_MIXER_EXT\n");
		return 0;
	}

	int mixerReturnVal = Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 1024);
	return mixerReturnVal;
}

void Sound::Enable(int channelFrom, int channelTo, int enableFlag)
{
	enabled_flag = enableFlag ? -1 : 0;
}

void Sound::Activate()
{
	Mix_Resume(-1);
}

void Sound::Deactivate()
{
	Mix_Pause(-1);
}

void Sound::Close()
{
	Mix_Quit();
}

void Sound::PlaySound(Mix_Chunk* wavePtr, int minChannel, int maxChannel, unsigned int dwFlags, int16_t loops)
{
	if (enabled_flag)
		Mix_PlayChannel(-1, wavePtr, loops);
}

Mix_Chunk* Sound::LoadWaveFile(std::string lpName)
{
	return Mix_LoadWAV(lpName.c_str());
}

void Sound::FreeSound(Mix_Chunk* wave)
{
	if (wave)
		Mix_FreeChunk(wave);
}
