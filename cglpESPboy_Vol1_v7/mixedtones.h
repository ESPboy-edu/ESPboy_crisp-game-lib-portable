//this file has been generated with the help of claude.ai

#ifndef MIXEDTONES_H
#define MIXEDTONES_H

#include <stdint.h>

// Setup functions
// pin_speaker_enable is unused on ESPboy (no enable pin), kept for API compatibility
void setupAudio(int32_t sample_rate, uint8_t pin_piezo);

// Core functions
void updateAudio();
int8_t playTone(float frequency, uint8_t volume, float duration_sec = 0, float delay_sec = 0);
int8_t playToneOnChannel(uint8_t channel, float frequency, uint8_t volume, float duration_sec = 0, float delay_sec = 0);
void stopChannel(int8_t channel);
void stopAllTones();
void cancelScheduled(int8_t channel);

// Query functions
uint8_t getMaxChannels();
int8_t findFreeChannel();
bool isChannelActive(int8_t channel);
uint8_t getActiveChannelCount();
uint8_t getPlayingChannelCount();
uint32_t getAudioStartTime();

#endif
