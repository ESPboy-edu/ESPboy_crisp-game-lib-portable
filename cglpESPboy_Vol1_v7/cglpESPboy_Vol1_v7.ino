#include "lib/ESPboyInit.h"
#include "lib/ESPboyInit.cpp"

//#include "lib/ESPboyTerminalGUI.h"
//#include "lib/ESPboyTerminalGUI.cpp"
//#include "lib/ESPboyOTA2.h"
//#include "lib/ESPboyOTA2.cpp"

#include "cglp.h"
#include "machineDependent.h"
#include "mixedtones.h"
#include <ESP_EEPROM.h>

ESPboyInit myESPboy;
//ESPboyTerminalGUI *terminalGUIobj = NULL;
//ESPboyOTA2 *OTA2obj = NULL;

uint8_t rePalette=1;
static LGFX_Sprite canvas(&myESPboy.tft);

#define TONE_PER_NOTE 32
#define SAMPLE_RATE 22000
#define SOUND_PIN D3

// Fixed-point scale in 16.16 format: 1.0 == FIXED_ONE
#define FIXED_ONE   (1 << 16)
#define FIXED_MUL(a, b) (((a) >> 8) * ((b) >> 8))
#define FIXED_CEIL(a)   (((a) + FIXED_ONE - 1) >> 16)
#define FLOAT_TO_FIXED(f) ((int32_t)((f) * FIXED_ONE))
#define FIXED_TO_INT(a)  ((a) >> 16)

static int canvasX;
static int canvasY;
static int viewW;
static int viewH;
static int offsetX;
static int offsetY;
static float mouseX;
static float mouseY;
static float mouseStepX;
static float mouseStepY;
static int32_t scale;  // 16.16 fixed-point
static unsigned long lastFrameTime;

#define HIGH_SCORE_QUANT 16
#define HIGH_SCORE_OFFSET 10
#define HIGH_SCORE_SAVE_ID 0x1234
long highScores[HIGH_SCORE_QUANT];

void md_loadHighScores() {
  uint16_t magic;
  EEPROM.get(0, magic);
  
  if (magic == HIGH_SCORE_SAVE_ID) { 
    for(int i = 0; i < HIGH_SCORE_QUANT; i++) {
      EEPROM.get(HIGH_SCORE_OFFSET + sizeof(long) * i, highScores[i]);
      if (highScores[i] < 0 || highScores[i] > 9999999) highScores[i] = 0;
    }
  } else {
    EEPROM.put(0, (uint16_t)HIGH_SCORE_SAVE_ID);
    for(int i = 0; i < HIGH_SCORE_QUANT; i++) {
      highScores[i] = 0;
      EEPROM.put(HIGH_SCORE_OFFSET + sizeof(long) * i, highScores[i]);
    }
    EEPROM.commit();
  }
}

void md_saveHighScore(int gameIndex, int score) {
  if (gameIndex < 0 || gameIndex >= HIGH_SCORE_QUANT) return;
  highScores[gameIndex] = score;
  EEPROM.put(HIGH_SCORE_OFFSET + sizeof(long) * gameIndex, score);
  EEPROM.commit();
}

void md_drawRect(float x, float y, float w, float h, unsigned char colorIndex) {
  if (scale == FIXED_ONE)
    canvas.fillRect(offsetX + x, offsetY + y, w, h, colorIndex);
  else 
    canvas.fillRect(offsetX + FIXED_TO_INT(FIXED_MUL(FLOAT_TO_FIXED(x), scale)),
                  offsetY + FIXED_TO_INT(FIXED_MUL(FLOAT_TO_FIXED(y), scale)),
                  FIXED_TO_INT(FIXED_MUL(FLOAT_TO_FIXED(w), scale)),
                  FIXED_TO_INT(FIXED_MUL(FLOAT_TO_FIXED(h), scale)),
                  colorIndex);
}


void md_drawCharacter(unsigned char (*grid)[CHARACTER_HEIGHT][CHARACTER_WIDTH], float x, float y) {
  uint8_t col;

  if (scale == FIXED_ONE) {
    int baseX = offsetX + (int)x;
    int baseY = offsetY + (int)y;

    for (uint8_t yy = 0; yy < CHARACTER_HEIGHT; yy++) {
      int py = baseY + yy; // Считаем Y строки один раз
      for (uint8_t xx = 0; xx < CHARACTER_WIDTH; xx++) {
        col = (*grid)[yy][xx];
        if (col != TRANSPARENT) {
          canvas.drawPixel(baseX + xx, py, col);
        }
      }
    }
  } else {
    int32_t fx = FLOAT_TO_FIXED(x);
    int32_t fy = FLOAT_TO_FIXED(y);
    int cellSize = FIXED_CEIL(scale);

    for (uint8_t yy = 0; yy < CHARACTER_HEIGHT; yy++) {
      int py = offsetY + FIXED_TO_INT(FIXED_MUL(fy + ((int32_t)yy << 16), scale));
      for (uint8_t xx = 0; xx < CHARACTER_WIDTH; xx++) {
        col = (*grid)[yy][xx];
        if (col != TRANSPARENT) {
          int px = offsetX + FIXED_TO_INT(FIXED_MUL(fx + ((int32_t)xx << 16), scale));
          canvas.drawPixel(px, py, col);
          // canvas.fillRect(px, py, cellSize, cellSize, col);
        }
      }
    }
  }
}


void md_clearView(unsigned char colorIndex) {
  canvas.clear(colorIndex);
  //canvas.fillRect(offsetX, offsetY, viewW, viewH, colorIndex);
}

void md_clearScreen(unsigned char colorIndex) {
  //canvas.clearClipRect();
  canvas.clear(colorIndex);
  //canvas.setClipRect(offsetX, offsetY, viewW, viewH);
  myESPboy.tft.fillScreen(myESPboy.tft.color565(colorRgbs[colorIndex].r, colorRgbs[colorIndex].g, colorRgbs[colorIndex].b));
}

void md_playTone(float freq, float duration, float when)
{ 
    float now = (millis() - getAudioStartTime()) / 1000.0;
    float delay_sec = when - now;
    
    if (delay_sec < 0) delay_sec = 0;  // Play immediately if in the past
    
    playTone(freq, 255, duration, delay_sec); 
}

void md_stopTone()
{
    stopAllTones();
}

float md_getAudioTime()
{
    return (millis() - getAudioStartTime()) / 1000.0;
}


void md_consoleLog(char *msg) {
  //Serial.println(msg);
}


void md_initView(int w, int h) {
  static bool alreadyInit = false;
  if (alreadyInit == false) {
    alreadyInit = true;

    const int SCREEN_W = 128;
    const int SCREEN_H = 128;

    canvas.setColorDepth(4);
    canvas.createPalette();
    canvas.createSprite(SCREEN_W, SCREEN_H);
    
    canvasX = (myESPboy.tft.width() - SCREEN_W) / 2;
    canvasY = (myESPboy.tft.height() - SCREEN_H) / 2;
  }

  int32_t canvasW = canvas.width();
  int32_t canvasH = canvas.height();
  
  int32_t xScale = ((int32_t)canvasW << 16) / w;
  int32_t yScale = ((int32_t)canvasH << 16) / h;
  
  if (xScale >= 1 << 16 && yScale >= 1 << 16) {
    scale = 1 << 16;
  } else {
    scale = (xScale < yScale) ? xScale : yScale;
  }
  
  viewW = (int)(((int32_t)w * scale) >> 16);
  viewH = (int)(((int32_t)h * scale) >> 16);
  
  mouseX = viewW >> 1;
  mouseY = viewH >> 1;
  offsetX = (int)(canvasW - viewW) >> 1;
  offsetY = (int)(canvasH - viewH) >> 1;
  
  mouseStepX = canvasW / 100.0f;
  mouseStepY = canvasH / 100.0f;
  
  canvas.clearClipRect();
  canvas.setClipRect(offsetX, offsetY, viewW, viewH);
}

static void updateFromSoundTask() {
  //this does not actually play the sound
  //the timer isr is used for that in mixedtones.
  updateAudio();
}


static void updateFromFrameTask() {
  bool mouseUsed = getGame(currentGameIndex).usesMouse;
  uint8_t pressed_buttons = myESPboy.getKeys();


  setButtonState(!mouseUsed && (pressed_buttons & PAD_LEFT),
                 !mouseUsed && (pressed_buttons & PAD_RIGHT),
                 !mouseUsed && (pressed_buttons & PAD_UP),
                 !mouseUsed && (pressed_buttons & PAD_DOWN),
                 (pressed_buttons & PAD_ESC), (pressed_buttons & PAD_ACT));

  if (mouseUsed)
  {
    if ((pressed_buttons & PAD_RIGHT))
      mouseX += mouseStepX;

    if ((pressed_buttons & PAD_LEFT))
      mouseX -= mouseStepX;

    if ((pressed_buttons & PAD_UP))
      mouseY -= mouseStepY;

    if ((pressed_buttons & PAD_DOWN))
      mouseY += mouseStepY;

    mouseX = clamp(mouseX, 0, canvas.width() - 2 * offsetX - 1);
    mouseY = clamp(mouseY, 0, canvas.height() - 2 * offsetY - 1);
    if (scale == FIXED_ONE)
      setMousePos((int32_t)mouseX, (int32_t)mouseY);
    else
      setMousePos((int32_t)(mouseX * FIXED_ONE) / scale, (int32_t)(mouseY * FIXED_ONE) / scale);
  }
  updateFrame();

  if (mouseUsed)
  {
    uint16_t col = myESPboy.tft.color565(255, 105, 180);
    canvas.fillRect((int)(offsetX + (mouseX - 1)), (int)(offsetY + (mouseY - 1)), (int)(3), (int)(3), col);
  }
  ESP.wdtFeed();

  if (rePalette) {
    for (uint8_t i = 0; i < 16; i++) {
      canvas.setPaletteColor(i, colorRgbs[i].r, colorRgbs[i].g, colorRgbs[i].b);
    }
    rePalette = 0;
  }

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

  

void setup() {
  //Serial.begin(115200);
  EEPROM.begin(sizeof(long) * HIGH_SCORE_QUANT + HIGH_SCORE_OFFSET);
  myESPboy.begin("crisp-game-lib #1");

//Check OTA2
//  if (myESPboy.getKeys()&PAD_ACT || myESPboy.getKeys()&PAD_ESC) { 
//     terminalGUIobj = new ESPboyTerminalGUI(&myESPboy.tft, &myESPboy.mcp);
//     OTA2obj = new ESPboyOTA2(terminalGUIobj);}
  
  //Serial.println();
  //Serial.println(ESP.getFreeHeap());
  allocateArrays();
  md_loadHighScores();
  setupAudio(SAMPLE_RATE, SOUND_PIN);
  enableSound();
  initGame();
}


void loop() {    
 static uint16_t frameCnt=0;
 static uint32_t timeNow, counterFrame, counterSound, counterFPS;
 
  timeNow = millis();
  
  if(timeNow-counterFrame > 1000 / FPS){
     updateFromFrameTask();
     counterFrame = timeNow;
     frameCnt++;
  }

  if(timeNow-counterSound > 1000 / (tempo * TONE_PER_NOTE / 50)){
    updateFromSoundTask();
    counterSound = timeNow;
  }

  //if(timeNow-counterFPS > 1000){
    //Serial.println(frameCnt);
  //  frameCnt = 0;
  //  counterFPS = timeNow;
  //}
}
