#ifndef MACHINE_DEPENDENT_H
#define MACHINE_DEPENDENT_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif

#include <Arduino.h>

#define CHARACTER_WIDTH 6
#define CHARACTER_HEIGHT 6

EXTERNC long highScores[];
EXTERNC void md_loadHighScores();
EXTERNC void md_saveHighScore(int gameIndex, int score);

EXTERNC void allocateOscilMemory();
EXTERNC void allocateParticleMemory();
EXTERNC void allocateArrays();
EXTERNC void md_drawRect(float x, float y, float w, float h, unsigned char colorIndex);
EXTERNC void md_drawCharacter(unsigned char (*grid)[CHARACTER_HEIGHT][CHARACTER_WIDTH], float x, float y);
EXTERNC void md_clearView(unsigned char colorIndex);
EXTERNC void md_clearScreen(unsigned char colorIndex);
EXTERNC void md_playTone(float freq, float duration, float when);
EXTERNC void md_stopTone();
EXTERNC float md_getAudioTime();
EXTERNC void md_initView(int w, int h);
EXTERNC void md_consoleLog(char *msg);

#endif
