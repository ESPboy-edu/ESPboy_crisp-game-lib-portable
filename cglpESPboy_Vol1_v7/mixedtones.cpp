//this file has been generated with the help of claude.ai
#include "machineDependent.h"
#include <stdint.h>
#include <Arduino.h>
#include <sigma_delta.h>
#include "mixedtones.h"

#define MAX_CHANNELS 16

static uint8_t  audioPinPiezo       = D3;
static int32_t  sampleRate          = 22050;
static uint32_t audioStartTime      = 0;

struct Oscillator
{
    uint32_t phase;
    uint32_t phase_increment;
    uint16_t amplitude;
    uint32_t duration_samples;
    uint32_t samples_played;
    uint32_t start_time_ms;
    bool     active;
    bool     scheduled;
};

//static Oscillator oscillators[MAX_CHANNELS];
static Oscillator *oscillators;

void allocateOscilMemory() {
  oscillators = (Oscillator*)malloc(sizeof(Oscillator) * MAX_CHANNELS);
}


static void ICACHE_RAM_ATTR timerISR()
{
    noInterrupts();
    
    uint32_t mixed_sample = 0;
    uint8_t active_count = 0;

    for (int i = 0; i < MAX_CHANNELS; i++)
    {
        if (!oscillators[i].active) continue;

        oscillators[i].phase += oscillators[i].phase_increment;
        oscillators[i].samples_played++;

        if (oscillators[i].duration_samples > 0 &&
            oscillators[i].samples_played >= oscillators[i].duration_samples)
        {
            oscillators[i].active    = false;
            oscillators[i].scheduled = false;
        }
        else if (oscillators[i].amplitude > 0)
        {
            active_count++;
            if ((oscillators[i].phase & 0x80000000) == 0) {
                mixed_sample += oscillators[i].amplitude;
            }
        }
    }

    uint8_t out = 0;
    
    if (active_count > 0)
    {
        static const uint16_t inv_count[17] = {
            0, 256, 128, 85, 64, 51, 42, 36, 32, 28, 25, 23, 21, 19, 18, 17, 16
        };
        mixed_sample = (mixed_sample * inv_count[active_count]) >> 8;
        out = (mixed_sample > 255) ? 255 : (uint8_t)mixed_sample;
    }

    sigmaDeltaWrite(0, out);
    
    interrupts();
}


// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

static inline void rebuildActiveChannelList() {}  // no-op, matches original API

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------

void setupAudio(int32_t sample_rate, uint8_t pin_piezo)
{
    audioPinPiezo = pin_piezo;
    sampleRate    = sample_rate;

    for (int i = 0; i < MAX_CHANNELS; i++)
    {
        oscillators[i].active           = false;
        oscillators[i].scheduled        = false;
        oscillators[i].phase            = 0;
        oscillators[i].amplitude        = 0;
        oscillators[i].duration_samples = 0;
        oscillators[i].samples_played   = 0;
        oscillators[i].start_time_ms    = 0;
    }


    noInterrupts();
    sigmaDeltaSetup(0, sample_rate*4);
    sigmaDeltaAttachPin(pin_piezo);
    sigmaDeltaEnable();
    timer1_attachInterrupt(timerISR);
    timer1_enable(TIM_DIV1, TIM_EDGE, TIM_LOOP);
    timer1_write(80000000 / sample_rate);
    sigmaDeltaWrite(0, 0); 
    interrupts();
    
    audioStartTime = millis();
}

uint8_t getMaxChannels() { return MAX_CHANNELS; }

int8_t findFreeChannel()
{
    for (int i = 0; i < MAX_CHANNELS; i++)
        if (!oscillators[i].active && !oscillators[i].scheduled)
            return (int8_t)i;
    //Serial.println("NoCh");
    return -1;
}

void updateAudio()
{
    uint32_t now = millis();
    for (int i = 0; i < MAX_CHANNELS; i++)
    {
        if (oscillators[i].scheduled && !oscillators[i].active)
        {
            if (now >= oscillators[i].start_time_ms)
            {
                oscillators[i].phase          = 0;
                oscillators[i].samples_played = 0;
                oscillators[i].active         = true;
                oscillators[i].scheduled      = false;
            }
        }
    }
    rebuildActiveChannelList();
}

static void setupChannel(uint8_t ch, float frequency, uint8_t volume,
                          float duration_sec, float delay_sec)
{
    oscillators[ch].phase_increment  =
        (uint32_t)((frequency * 4294967296.0f) / (float)sampleRate);
    oscillators[ch].amplitude        = volume;
    oscillators[ch].duration_samples =
        (duration_sec > 0.0f) ? (uint32_t)((float)sampleRate * duration_sec) : 0;
    oscillators[ch].phase            = 0;
    oscillators[ch].samples_played   = 0;

    if (delay_sec > 0.0f)
    {
        oscillators[ch].start_time_ms = millis() + (uint32_t)(delay_sec * 1000.0f);
        oscillators[ch].scheduled     = true;
        oscillators[ch].active        = false;
    }
    else
    {
        oscillators[ch].start_time_ms = 0;
        oscillators[ch].scheduled     = false;
        oscillators[ch].active        = true;
    }
}

int8_t playTone(float frequency, uint8_t volume, float duration_sec, float delay_sec)
{
    int8_t ch = findFreeChannel();
    if (ch < 0) return -1;
    setupChannel((uint8_t)ch, frequency, volume, duration_sec, delay_sec);
    rebuildActiveChannelList();
    return ch;
}

int8_t playToneOnChannel(uint8_t channel, float frequency, uint8_t volume,
                          float duration_sec, float delay_sec)
{
    if (channel >= MAX_CHANNELS) return -1;
    setupChannel(channel, frequency, volume, duration_sec, delay_sec);
    rebuildActiveChannelList();
    return (int8_t)channel;
}

void cancelScheduled(int8_t channel)
{
    if (channel >= 0 && channel < MAX_CHANNELS)
    {
        oscillators[channel].scheduled = false;
        oscillators[channel].active    = false;
    }
}

void stopChannel(int8_t channel)
{
    if (channel >= 0 && channel < MAX_CHANNELS)
    {
        oscillators[channel].active    = false;
        oscillators[channel].scheduled = false;
        oscillators[channel].amplitude = 0;
    }
}

void stopAllTones()
{
    for (int i = 0; i < MAX_CHANNELS; i++)
    {
        oscillators[i].active    = false;
        oscillators[i].scheduled = false;
        oscillators[i].amplitude = 0;
    }
    sigmaDeltaWrite(0, 0);
}

bool isChannelActive(int8_t channel)
{
    if (channel < 0 || channel >= MAX_CHANNELS) return false;
    return oscillators[channel].active || oscillators[channel].scheduled;
}

uint8_t getActiveChannelCount()
{
    uint8_t count = 0;
    for (int i = 0; i < MAX_CHANNELS; i++)
        if (oscillators[i].active || oscillators[i].scheduled) count++;
    return count;
}

uint8_t getPlayingChannelCount()
{
    uint8_t count = 0;
    for (int i = 0; i < MAX_CHANNELS; i++)
        if (oscillators[i].active) count++;
    return count;
}

uint32_t getAudioStartTime() { return audioStartTime; }
