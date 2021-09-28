#pragma once


class Sound
{
public:
	static int Init(int voices, int curMidiBackend, int adlEmu = ADLMIDI_OPL3_EMU_DEFAULT, int opnEmu = OPNMIDI_OPN2_EMU_DEFAULT);
	static void Enable(int channelFrom, int channelTo, int enableFlag);
	static void Activate();
	static void Deactivate();
	static void Close();
	static void PlaySound(Mix_Chunk* wavePtr, int minChannel, int maxChannel, unsigned int dwFlags, int16_t loops);
	static Mix_Chunk* LoadWaveFile(std::string lpName);
	static void FreeSound(Mix_Chunk* wave);
private:
	static int num_channels;
	static unsigned int enabled_flag;
	static const int initFlags;
};
