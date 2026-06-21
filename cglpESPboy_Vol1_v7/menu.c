#include "menu.h"
#include "cglp.h"

#define MAX_GAME_COUNT 11
int gameCount = 0;
static Game games[MAX_GAME_COUNT];
static int gameIndex = 1;

static void update() {
  color = BLACK;
  text("[A]Select[B]Sound", 1, 2);
  if (currentInput.b.isJustPressed) {
      toggleSound();
  }
  if (/*input.b.isJustPressed || */input.down.isJustPressed) {
    gameIndex++;
  }
  if (input.up.isJustPressed) {
    gameIndex--;
  }
  gameIndex = wrap(gameIndex, 1, gameCount);
  for (int i = 0; i < gameCount; i++) {
    color = BLACK;
    float y = i * 6;
    if (i == gameIndex) {
      rect(0, y+2, 100, 5);
      color = WHITE;
    }
    text(games[i].title, 3, y + 4);

    if (i > 0 && strlen(games[i].title) > 0) {
      char *hsStr = intToChar(highScores[i]);
      int strW = strlen(hsStr) * 6;
      text(hsStr, 100 - strW - 2, y + 4);
    }
    
  }
  if (input.a.isJustPressed) {
    restartGame(gameIndex);
  }
}

void addGame(char *title, char *description,
             char (*characters)[CHARACTER_WIDTH][CHARACTER_HEIGHT + 1],
             int charactersCount, Options options, int usesMouse, void (*update)(void)) {
  if (gameCount >= MAX_GAME_COUNT) {
    return;
  }
  Game *g = &games[gameCount];
  g->title = title;
  g->description = description;
  g->characters = characters;
  g->charactersCount = charactersCount;
  g->options = options;
  g->usesMouse = usesMouse;
  g->update = update;
  gameCount++;
}

Game getGame(int index) { return games[index]; }

void addMenu() {
  Options o = {
      .viewSizeX = 100, .viewSizeY = 100, .soundSeed = 0, .isDarkColor = false};
  addGame("", "", NULL, 0, o, false, update);
}
