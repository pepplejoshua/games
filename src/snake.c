#include "../lib/raylib.h"

#define TILE_SIZE 16
#define ROWS 30
#define COLS 20

static short foodPos[2], snakePos[2],
    snakeDir = 0, snakePartTime[ROWS][COLS] = {0}, snakeLength = 2, score = 0,
    gameOver = 0;

int main() {
  InitWindow(ROWS * TILE_SIZE, COLS * TILE_SIZE, "snake game");
  foodPos[0] = GetRandomValue(0, ROWS - 1);
  foodPos[1] = GetRandomValue(0, COLS - 1);
  snakePos[0] = (int)ROWS / 2, snakePos[1] = (int)COLS / 2;
  SetTargetFPS(7);
  while (!WindowShouldClose()) {
    if (gameOver == 0) {
      snakePos[0] += (snakeDir == 0) ? 1 : ((snakeDir == 1) ? -1 : 0);
      snakePos[1] += (snakeDir == 2) ? 1 : ((snakeDir == 3) ? -1 : 0);
      // check if the snake has collided with itself due to its new position
      if (snakePartTime[snakePos[0]][snakePos[1]] > 0)
        gameOver = 1;

      snakePartTime[snakePos[0]][snakePos[1]] = snakeLength;
      for (short i = 0; i < ROWS; i++)
        for (int j = 0; j < COLS; j++)
          snakePartTime[i][j] -= (snakePartTime[i][j] > 0) ? 1 : 0;

      snakeDir = IsKeyDown(KEY_LEFT)
                     ? 1
                     : (IsKeyDown(KEY_RIGHT)
                            ? 0
                            : (IsKeyDown(KEY_UP)
                                   ? 3
                                   : (IsKeyDown(KEY_DOWN) ? 2 : snakeDir)));
      if ((snakePos[0] < 0) || (snakePos[1] < 0) || (snakePos[0] >= ROWS) ||
          (snakePos[1] >= COLS))
        gameOver = 1;

      if ((snakePos[0] == foodPos[0]) && (snakePos[1] == foodPos[1])) {
        score += 100;
        snakeLength++;
        foodPos[0] = GetRandomValue(0, ROWS - 1);
        foodPos[1] = GetRandomValue(0, COLS - 1);
      }
    }

    BeginDrawing();
    {
      ClearBackground(RAYWHITE);
      for (int i = 0; i < GetScreenWidth() / TILE_SIZE + 1; i++)
        DrawLineV((Vector2){TILE_SIZE * i, 0},
                  (Vector2){TILE_SIZE * i, GetScreenHeight()}, LIGHTGRAY);

      for (int i = 0; i < GetScreenHeight() / TILE_SIZE + 1; i++)
        DrawLineV((Vector2){0, TILE_SIZE * i},
                  (Vector2){GetScreenWidth(), TILE_SIZE * i}, LIGHTGRAY);

      if (gameOver == 0) {
        DrawRectangle(foodPos[0] * TILE_SIZE, foodPos[1] * TILE_SIZE, TILE_SIZE,
                      TILE_SIZE, RED);
        for (short i = 0; i < ROWS; i++)
          for (int j = 0; j < COLS; j++)
            if (snakePartTime[i][j] > 0)
              DrawRectangle(i * TILE_SIZE, j * TILE_SIZE, TILE_SIZE, TILE_SIZE,
                            GREEN);

        DrawText(TextFormat("SCORE: %i", score), TILE_SIZE, TILE_SIZE - 2, 20,
                 BLACK);
      } else
        DrawText("YOU LOST!",
                 GetScreenWidth() / 2 - (MeasureText("YOU LOST!", 40)),
                 COLS * TILE_SIZE / 4, 40, RED);
    }
    EndDrawing();
  }
  CloseWindow();
  return 0;
}
