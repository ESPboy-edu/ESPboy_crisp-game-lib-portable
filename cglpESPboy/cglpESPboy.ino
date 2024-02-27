#include "lib/ESPboyInit.h"
#include "lib/ESPboyInit.cpp"
#include <Ticker.h>
#include "cglp.h"
#include "machineDependent.h"

#define FLT_MAX __FLT_MAX__
#define TRANSPARENT_COLOR 0

ESPboyInit myESPboy;
Ticker updateFrameTick, updateSoundTick;

static LGFX_Sprite canvas(&myESPboy.tft);
static LGFX_Sprite characterSprite(&myESPboy.tft);

static int canvasX;
static int canvasY;

typedef struct {
  float freq;
  float duration;
  float when;
} SoundTone;

#define TONE_PER_NOTE 16
#define SOUND_TONE_COUNT 32
static SoundTone soundTones[SOUND_TONE_COUNT];
static int soundToneIndex = 0;
static float soundTime = 0;

static void initSoundTones() {
  for (int i = 0; i < SOUND_TONE_COUNT; i++) {
    soundTones[i].when = FLT_MAX;
  }
}

static void addSoundTone(float freq, float duration, float when) {
  SoundTone *st = &soundTones[soundToneIndex];
  st->freq = freq;
  st->duration = duration;
  st->when = when;
  soundToneIndex++;
  if (soundToneIndex >= SOUND_TONE_COUNT) {
    soundToneIndex = 0;
  }
}

void md_drawRect(float x, float y, float w, float h, unsigned char r, unsigned char g, unsigned char b) {
  canvas.fillRect((int)x, (int)y, (int)w, (int)h, myESPboy.tft.color565(r, g, b));
}


void md_drawCharacter(unsigned char grid[CHARACTER_HEIGHT][CHARACTER_WIDTH][3], float x, float y) {
  static uint16_t characterImageData[CHARACTER_WIDTH * CHARACTER_HEIGHT];
  int cp = 0;
  for (int y = 0; y < CHARACTER_HEIGHT; y++) {
    for (int x = 0; x < CHARACTER_WIDTH; x++) {
      unsigned char r = grid[y][x][0];
      unsigned char g = grid[y][x][1];
      unsigned char b = grid[y][x][2];
      if(r|g|b) characterImageData[cp] = myESPboy.tft.color565(r, g, b);
      else characterImageData[cp] = TRANSPARENT_COLOR;
      cp++;
    }
  } 
  characterSprite.pushImage(0, 0, CHARACTER_WIDTH, CHARACTER_HEIGHT, characterImageData);
  characterSprite.pushSprite(&canvas, (int)x, (int)y, TRANSPARENT_COLOR);
}


void md_clearView(unsigned char r, unsigned char g, unsigned char b) {
  canvas.fillSprite(myESPboy.tft.color565(r, g, b));
}

void md_clearScreen(unsigned char r, unsigned char g, unsigned char b) {
  myESPboy.tft.fillScreen(myESPboy.tft.color565(r, g, b));
}

void md_playTone(float freq, float duration, float when) {
  addSoundTone(freq, duration, when);
}

void md_stopTone() {
  initSoundTones();
  myESPboy.noPlayTone();
}

float md_getAudioTime() { return soundTime; }

void md_consoleLog(char *msg) { Serial.println(msg); }

void md_initView(int w, int h) {
  static bool alreadyInit = false;
  if (alreadyInit == false){
    alreadyInit = true;
    canvas.createSprite(w, h);
    characterSprite.createSprite(CHARACTER_WIDTH, CHARACTER_HEIGHT);
    characterSprite.setSwapBytes(true);
    canvasX = (myESPboy.tft.width() - w) / 2;
    canvasY = (myESPboy.tft.height() - h) / 2;
  }
}


static void updateFromFrameTask() {
  uint8_t readKeys = myESPboy.getKeys();
  setButtonState(readKeys&PAD_LEFT, readKeys&PAD_RIGHT, readKeys&PAD_UP, readKeys&PAD_DOWN, readKeys&PAD_ESC, readKeys&PAD_ACT);
  updateFrame();
  ESP.wdtFeed();
  canvas.pushSprite(canvasX, canvasY);
  if (!isInMenu) {
    if (currentInput.b.isJustPressed) {
      if (currentInput.a.isPressed) {
        goToMenu();
      } else {
        toggleSound();
      }
    }
  }
}


static void updateFromSoundTask() {
  soundTime += 60 / tempo / TONE_PER_NOTE;
  float lastWhen = 0;
  int ti = -1;
  for (int i = 0; i < SOUND_TONE_COUNT; i++) {
    SoundTone *st = &soundTones[i];
    if (st->when <= soundTime) {
      if (st->when > lastWhen) {
        ti = i;
        lastWhen = st->when;
        st->when = FLT_MAX;
      }
    }
  }
  if (ti >= 0) {
    SoundTone *st = &soundTones[ti];
    myESPboy.playTone((uint16_t)st->freq, (uint32_t)(st->duration * 1000));
  }
}



void setup() {
  //Serial.begin(115200);
  myESPboy.begin("crisp-game-lib v1.1");
  //Serial.println();
  //Serial.println(ESP.getFreeHeap());
  initSoundTones();
  enableSound();
  //disableSound();
  initGame();

  updateFrameTick.attach_ms(1000/FPS, updateFromFrameTask);
  updateSoundTick.attach_ms(1000/(tempo * TONE_PER_NOTE / 50.0f), updateFromSoundTask);  
}

void loop() { 
  delay(1000); 
  //Serial.println(ESP.getFreeHeap());
}
