// author: Jan Rudnicki
// game: Snake in SDL2
// version: 1.0
// komentarz: Używałem Szablonu SDL do drugiego projektu

#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS

#include<time.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<math.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

// ------------------
// DEFINING CONSTANTS
// ------------------

#define SCREEN_WIDTH 640 // pixels
#define SCREEN_HEIGHT 440 // pixels

#define INFO_AREA_HEIGHT (SCREEN_HEIGHT / 11) // The height of the area where all the information is displayed
#define GAME_BOARD_HEIGHT (SCREEN_HEIGHT - INFO_AREA_HEIGHT) // The height of the game board

#define CELL_SIZE       20 // The size of a single cell in the game board
#define ROW_CELLS       (SCREEN_WIDTH / CELL_SIZE) // The number of cells in a row
#define COL_CELLS       (GAME_BOARD_HEIGHT / CELL_SIZE) // The number of cells in a column

// The direction numbers
#define RIGHT 0
#define LEFT 1
#define UP 2
#define DOWN 3

#define SNAKE_LENGTH 1; // The initial length of the snake

#define SPEEDUP 0.8; // How much a snake shoudl speedup after a certain time (1-SPEEDUP)% of the current speed
const int SPEEDUP_TIME = 5; // The time innterval after which the snake speeds up (whole seconds)

// -------------------
// DEFINING STRUCTURES
// -------------------

struct Segment {
        int x;
        int y;
};

struct Snake {
        Segment* body; // creating an array of the whole snake body
        int length; // length of the snake
        int direction; // direction of the snake
        double speed; // speed of the snake
};

struct Dot {
        int x;
        int y;
};

// --------------
// DRAW FUNCTIONS
// --------------

// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface* screen, int x, int y, const char* text, SDL_Surface* charset) {
        int px, py, c;
        SDL_Rect s, d;
        s.w = 8;
        s.h = 8;
        d.w = 8;
        d.h = 8;
        while (*text) {
                c = *text & 255;
                px = (c % 16) * 8;
                py = (c / 16) * 8;
                s.x = px;
                s.y = py;
                d.x = x;
                d.y = y;
                SDL_BlitSurface(charset, &s, screen, &d);
                x += 8;
                text++;
        }
}

// draw a surface sprite on a surface screen in point (x, y)
// (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
        SDL_Rect dest;
        dest.x = x - sprite->w / 2;
        dest.y = y - sprite->h / 2;
        dest.w = sprite->w;
        dest.h = sprite->h;
        SDL_BlitSurface(sprite, NULL, screen, &dest);
}

// draw a single pixel
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
        int bpp = surface->format->BytesPerPixel;
        Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
        *(Uint32*)p = color;
}

// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
        for (int i = 0; i < l; i++) {
                DrawPixel(screen, x, y, color);
                x += dx;
                y += dy;
        }
}

// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) {
        int i;
        DrawLine(screen, x, y, k, 0, 1, outlineColor);
        DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
        DrawLine(screen, x, y, l, 1, 0, outlineColor);
        DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
        for (i = y + 1; i < y + k - 1; i++) {
                DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
        }
}

// Function to draw the dot on the game board
void DrawDot(SDL_Surface* screen, SDL_Surface* dotSurface, Dot* blueDot) {
        int x = blueDot->x * CELL_SIZE;
        int y = INFO_AREA_HEIGHT + blueDot->y * CELL_SIZE;
        SDL_Rect dest;
        dest.x = x;
        dest.y = y;
        dest.w = CELL_SIZE;
        dest.h = CELL_SIZE;
        SDL_BlitScaled(dotSurface, NULL, screen, &dest);
}

// Function to draw the grey grid on the game board
void DrawGrid(SDL_Surface* screen, Uint32 gridColor) {
        // vertical lines
        for (int x = 0; x <= SCREEN_WIDTH; x += CELL_SIZE) {
                DrawLine(screen, x, INFO_AREA_HEIGHT, GAME_BOARD_HEIGHT, 0, 1, gridColor);
        }
        // horizontal lines
        for (int y = INFO_AREA_HEIGHT; y <= SCREEN_HEIGHT; y += CELL_SIZE) {
                DrawLine(screen, 0, y, SCREEN_WIDTH, 1, 0, gridColor);
        }
}

// ---------------
// SNAKE FUNCTIONS
// ---------------

// Function to Initialize the snake
void InitSnake(Snake* snake) {
        snake->body = new Segment[ROW_CELLS * COL_CELLS];
        snake->length = SNAKE_LENGTH;
        for (int i = 0; i < snake->length; i++) {
                snake->body[i].x = ROW_CELLS / 2 - i;
                snake->body[i].y = COL_CELLS / 2;
        }
        snake->direction = RIGHT;
        snake->speed = 0.2; // move every x seconds
}

// Function to check if the snake can turn at the border
bool canTurn(Snake* snake, int direction) {
        if (direction == DOWN) {
                for (int i = 1; i < snake->length; i++) {
                        if (snake->body[0].x == snake->body[i].x && snake->body[0].y + 1 == snake->body[i].y) {
                                return false;
                        }
                }
                return true;
        }
        else if (direction == UP) {
                for (int i = 1; i < snake->length; i++) {
                        if (snake->body[0].x == snake->body[i].x && snake->body[0].y - 1 == snake->body[i].y) {
                                return false;
                        }
                }
                return true;
        }
        else if (direction == RIGHT) {
                for (int i = 1; i < snake->length; i++) {
                        if (snake->body[0].x + 1 == snake->body[i].x && snake->body[0].y == snake->body[i].y) {
                                return false;
                        }
                }
                return true;
        }
        else if (direction == LEFT) {
                for (int i = 1; i < snake->length; i++) {
                        if (snake->body[0].x - 1 == snake->body[i].x && snake->body[0].y == snake->body[i].y) {
                                return false;
                        }
                }
                return true;
        }
        return false;
}

// Function to add a segment to the snake when it eats the blue dot
void growSnake(Snake* snake) {
        snake->body[snake->length] = snake->body[snake->length - 1];
        snake->length++;
}

// Function to speed up the snake after a certain time
void SpeedUp(Snake* snake) {
        snake->speed *= SPEEDUP;
}

// Function to update the position of the snake
void UpdateSnake(Snake* snake) {
        for (int i = snake->length - 1; i > 0; i--) {
                snake->body[i].x = snake->body[i - 1].x;
                snake->body[i].y = snake->body[i - 1].y;
        }
        if (snake->direction == RIGHT) {
                if (snake->body[0].x == ROW_CELLS - 1) {
                        if (snake->body[0].y < COL_CELLS - 1 && canTurn(snake, DOWN)) {
                                snake->direction = DOWN;
                                snake->body[0].y++;
                        }
                        else {
                                snake->direction = UP;
                                snake->body[0].y--;
                        }
                }
                else {
                        snake->body[0].x++;
                }
        }
        else if (snake->direction == LEFT) {
                if (snake->body[0].x == 0) {
                        if (snake->body[0].y > 0 && canTurn(snake, UP)) {
                                snake->direction = UP;
                                snake->body[0].y--;
                        }
                        else {
                                snake->direction = DOWN;
                                snake->body[0].y++;
                        }
                }
                else {
                        snake->body[0].x--;
                }
        }
        else if (snake->direction == UP) {
                if (snake->body[0].y == 0) {
                        if (snake->body[0].x < ROW_CELLS - 1 && canTurn(snake, RIGHT)) {
                                snake->direction = RIGHT;
                                snake->body[0].x++;
                        }
                        else {
                                snake->direction = LEFT;
                                snake->body[0].x--;
                        }
                }
                else {
                        snake->body[0].y--;
                }
        }
        else if (snake->direction == DOWN) {
                if (snake->body[0].y == COL_CELLS - 1) {
                        if (snake->body[0].x > 0 && canTurn(snake, LEFT)) {
                                snake->direction = LEFT;
                                snake->body[0].x--;
                        }
                        else {
                                snake->direction = RIGHT;
                                snake->body[0].x++;
                        }
                }
                else {
                        snake->body[0].y++;
                }
        }
}

// Function to Draw the snake
void DrawSnake(SDL_Surface* screen, Snake* snake, Uint32 headColor, Uint32 bodyColor, Uint32 borderColor) {
        for (int i = snake->length - 1; i >= 0; i--) {
                int x = snake->body[i].x * CELL_SIZE;
                int y = INFO_AREA_HEIGHT + snake->body[i].y * CELL_SIZE;
                if (i == 0) {
                        // Draw the head of the snake (headColor)
                        DrawRectangle(screen, x, y, CELL_SIZE, CELL_SIZE, borderColor, headColor);
                }
                else {
                        // Draw the rest of the snake (bodyColor)
                        DrawRectangle(screen, x, y, CELL_SIZE, CELL_SIZE, borderColor, bodyColor);
                }
        }
}

// Function to check if the snake has collided with itself
bool checkCollision(Snake* snake) {
        for (int i = 2; i < snake->length; i++) {
                if (snake->body[0].x == snake->body[i].x && snake->body[0].y == snake->body[i].y) {
                        return true;
                }
        }
        return false;
}

// -------------
// DOT FUNCTIONS
// -------------

// Function to initialize the blue dot
void InitDot(Dot* blueDot, Snake* snake) {
        bool flag = false;
        while (!flag) {
                flag = true;
                blueDot->x = rand() % ROW_CELLS;
                blueDot->y = rand() % COL_CELLS;
                for (int i = 0; i < snake->length; i++) {
                        if (snake->body[i].x == blueDot->x && snake->body[i].y == blueDot->y) {
                                flag = false;
                                break;
                        }
                }
        }
}

// Function to check if the snake has eaten the blue dot
bool checkDotCollision(Dot* blueDot, Snake* snake) {
        if (snake->body[0].x == blueDot->x && snake->body[0].y == blueDot->y) {
                return true;
        }
        return false;
}

// --------------
// GAME FUNCTIONS
// --------------

// Function the GameOver message
void DisplayGameOver(SDL_Surface* screen, SDL_Surface* charset, char* text) {
        sprintf(text, "Game Over!");
        DrawString(screen, screen->w / 2 - strlen(text) * 4, 45, text, charset);

        sprintf(text, "Press 'n' for a new game or 'Esc' to exit");
        DrawString(screen, screen->w / 2 - strlen(text) * 4, 60, text, charset);
}

// Function to display the game information
void DisplayInfoText(SDL_Surface* screen, SDL_Surface* charset, char* text, double worldTime) {
        sprintf(text, "Jan Rudnicki 203179 - Snake, Time = %.1lf s, Implemented Requirements: 1-4,A,B", worldTime);
        DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);
        sprintf(text, "Esc - exit, n - new game, Move the snake using arrow keys");
        DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);
}

// Function to free all the memory
void FreeMemory(Snake* snake, Dot* blueDot) {
        delete[] snake->body;
        delete snake;
        delete blueDot;
}

// -------------
// MAIN FUNCTION
// -------------

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char** argv) {

        srand(time(NULL));

        SDL_Event event;
        SDL_Window* window;
        SDL_Renderer* renderer;

        // Preparing the SDL screen
        if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
                printf("SDL_Init error: %s\n", SDL_GetError());
                return 1;
        }

        // rc = SDL_CreateWindowAndRenderer(0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP, &window, &renderer);
        int rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
        if (rc != 0) {
                SDL_Quit();
                printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
                return 1;
        }

        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
        SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        SDL_SetWindowTitle(window, "Snake Game");

        SDL_Surface* screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
        SDL_Texture* scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);

        // turning of the cursor
        SDL_ShowCursor(SDL_DISABLE);

        SDL_Surface* charset = SDL_LoadBMP("./cs8x8.bmp");
        if (charset == NULL) {
                printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
                SDL_FreeSurface(screen);
                SDL_DestroyTexture(scrtex);
                SDL_DestroyWindow(window);
                SDL_DestroyRenderer(renderer);
                SDL_Quit();
                return 1;
        }
        SDL_SetColorKey(charset, true, 0x000000);

        SDL_Surface* blueDotSurface = SDL_LoadBMP("./blue_dot.bmp");
        if (blueDotSurface == NULL) {
                printf("SDL_LoadBMP(blue_dot.bmp) error: %s\n", SDL_GetError());
                SDL_FreeSurface(screen);
                SDL_FreeSurface(charset);
                SDL_DestroyTexture(scrtex);
                SDL_DestroyWindow(window);
                SDL_DestroyRenderer(renderer);
                SDL_Quit();
                return 1;
        }
        SDL_SetColorKey(blueDotSurface, true, 0x000000);

        // Initialize the colors
        int czarny = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
        int bialy = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);
        int szary = SDL_MapRGB(screen->format, 0x80, 0x80, 0x80);
        int czerwony = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
        int zielony = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
        int niebieski = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);

        // Text variables
        char text[128];

        // Time variables
        double worldTime = 0;
        double snakeTime = 0;
        double speedupTime = 0;
        int t1 = SDL_GetTicks();
        int t2;
        double delta;

        // Game variables
        int canMove = 0;
        int gameOver = 0;
        int quit = 0;

        // Allocate memory for the snake
        Snake* snake = new Snake;
        InitSnake(snake);

        // Allocate memory for the blue dot
        Dot* blueDot = new Dot;
        InitDot(blueDot, snake);

        while (!quit) {

                t2 = SDL_GetTicks();
                delta = (t2 - t1) * 0.001; // get the time in seconds
                t1 = t2;

                if (!gameOver) {
                        worldTime += delta;
                        speedupTime += delta;
                        snakeTime += delta;
                        // If ennough time has passed since the last move, the snake moves
                        if (snakeTime >= snake->speed) {
                                UpdateSnake(snake);
                                canMove = 1;
                                snakeTime = 0;
                        }
                        // If enough time has passed since the last speedup, the snake speeds up
                        if (speedupTime >= SPEEDUP_TIME) {
                                SpeedUp(snake);
                                speedupTime = 0;
                        }
                        // Check if the snake has eaten the blue dot
                        if (checkDotCollision(blueDot, snake)) {
                                InitDot(blueDot, snake);
                                growSnake(snake);
                        }
                }

                // Clear the screen
                SDL_FillRect(screen, NULL, czarny);

                // Draw everything on the screen
                DrawRectangle(screen, 0, 0, SCREEN_WIDTH, INFO_AREA_HEIGHT, czarny, czerwony);
                DrawRectangle(screen, 0, INFO_AREA_HEIGHT, SCREEN_WIDTH, GAME_BOARD_HEIGHT, czarny, czarny);
                DrawGrid(screen, szary);
                DrawSnake(screen, snake, czerwony, zielony, bialy);
                DrawDot(screen, blueDotSurface, blueDot);

                // Check if the snake collided with itself
                if (checkCollision(snake)) {
                        gameOver = 1;
                        DisplayGameOver(screen, charset, text);
                }

                // Display the information text
                DisplayInfoText(screen, charset, text, worldTime);
                SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
                SDL_RenderCopy(renderer, scrtex, NULL, NULL);
                SDL_RenderPresent(renderer);

                while (SDL_PollEvent(&event)) {
                        switch (event.type) {
                        case SDL_KEYDOWN:
                                if (event.key.keysym.sym == SDLK_ESCAPE) {
                                        // If the escape is pressed, exit the game
                                        quit = 1;
                                }
                                else if (event.key.keysym.sym == SDLK_n) {
                                        // If the 'n' key is pressed, start a new game
                                        FreeMemory(snake, blueDot);
                                        snake = new Snake;
                                        InitSnake(snake);
                                        blueDot = new Dot;
                                        InitDot(blueDot, snake);
                                        gameOver = 0;
                                        worldTime = 0;
                                        snakeTime = 0;
                                        speedupTime = 0;
                                        canMove = 0;
                                }
                                else if (!gameOver && canMove) { //Checking if the game is not over and the snake can move
                                        if (event.key.keysym.sym == SDLK_RIGHT && (snake->direction != LEFT || snake->length==1)) {
                                                if (snake->body[0].x < ROW_CELLS - 1) {
                                                        snake->direction = RIGHT;
                                                        canMove = 0;
                                                }
                                        }
                                        else if (event.key.keysym.sym == SDLK_LEFT && (snake->direction != RIGHT || snake->length==1)) {
                                                if (snake->body[0].x > 0) {
                                                        snake->direction = LEFT;
                                                        canMove = 0;
                                                }
                                        }
                                        else if (event.key.keysym.sym == SDLK_UP && (snake->direction != DOWN || snake->length==1)) {
                                                if (snake->body[0].y > 0) {
                                                        snake->direction = UP;
                                                        canMove = 0;
                                                }
                                        }
                                        else if (event.key.keysym.sym == SDLK_DOWN && (snake->direction != UP || snake->length==1)) {
                                                if (snake->body[0].y < COL_CELLS - 1) {
                                                        snake->direction = DOWN;
                                                        canMove = 0;
                                                }
                                        }
                                }
                                break;
                        case SDL_QUIT:
                                quit = 1;
                                break;
                        }
                }
        }

        // Freeing all surfaces
        SDL_FreeSurface(charset);
        SDL_FreeSurface(screen);
        SDL_FreeSurface(blueDotSurface);
        SDL_DestroyTexture(scrtex);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);

        // Freeing all memory
        FreeMemory(snake, blueDot);

        // Quit SDL
        SDL_Quit();

        return 0;
}
