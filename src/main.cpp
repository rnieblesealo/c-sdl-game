#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#include <SDL_keycode.h>
#include <SDL_pixels.h>
#include <SDL_rect.h>
#include <SDL_render.h>
#include <SDL_timer.h>
#include <chrono>
#include <stdio.h>
#include <string.h>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;
const int GLOB_SCALE = 8;

SDL_Window *window = NULL;         // render target
SDL_Surface *screenSurface = NULL; // screen surface
SDL_Renderer *renderer = NULL;     // work with textures; hardware blitting

// we can initialize rects like this because they're structs
// structs are like arrays with unevenly sized elements
SDL_Rect screenRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

// inputs; used to check iskeydown
typedef enum Inputs { UP, DOWN, LEFT, RIGHT } Inputs;

const int KEY_COUNT = 4;
bool KEYS[KEY_COUNT];

bool Init() {
  // start sdl
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL init failed: %s\n", SDL_GetError());
    return false;
  }

  // make window
  window =
      SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (window == NULL) {
    printf("Window creation failed: %s\n", SDL_GetError());
    return false;
  }

  // make renderer for window
  renderer = SDL_CreateRenderer(
      window, -1,
      SDL_RENDERER_ACCELERATED); // we can | SDL_RENDERER_PRESENTVSYNC to enable
                                 // that, but don't; already have target fps
                                 // system

  if (renderer == NULL) {
    printf("Could not create renderer :%s\n", SDL_GetError());
    return false;
  }

  // adjust renderer color used for various operations (?)
  SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

  // start up sdl image loader
  int imageFlags = IMG_INIT_PNG; // this bitmask should result in a 1
  int initResult = IMG_Init(imageFlags) & imageFlags;
  if (!initResult) {
    printf("Could not load SDL image: %s\n", SDL_GetError());
    return false;
  }

  // get window surface
  screenSurface = SDL_GetWindowSurface(window);

  // set keys
  for (int i = 0; i < KEY_COUNT; ++i)
    KEYS[i] = false;

  return true;
}

// texture wrapper
class LTexture {
public:
  LTexture() {
    texture = NULL;
    width = 0;
    height = 0;
  }

  ~LTexture() { Free(); }

  bool LoadFromFile(const char *path) {
    // remove pre-existing texture
    Free();

    // load new one
    SDL_Surface *lSurf = IMG_Load(path);

    if (lSurf == NULL) {
      printf("Unable to load image: %s\n", SDL_GetError());
      return false;
    }

    // set color key to black
    SDL_SetColorKey(lSurf, SDL_TRUE,
                    SDL_MapRGB(lSurf->format, 0x00, 0x00, 0x00));

    // make texture w/color key
    SDL_Texture *nTexture = SDL_CreateTextureFromSurface(renderer, lSurf);

    if (nTexture == NULL) {
      printf("Could not create texture: %s\n", SDL_GetError());
      return false;
    }

    width = lSurf->w;
    height = lSurf->h;

    // get rid of interim surface
    SDL_FreeSurface(lSurf);

    // set new texture
    texture = nTexture;

    // set scale to global
    SetScale(GLOB_SCALE);

    return true;
  }

  void Free() {
    if (texture != NULL) {
      SDL_DestroyTexture(texture);
      texture = NULL;
      width = 0;
      height = 0;
    }
  }

  void SetBlendMode(SDL_BlendMode blendMode) {
    // set blend mode
    SDL_SetTextureBlendMode(texture, blendMode);
  }

  // modulation ~ multiplication!

  void ModColor(Uint8 r, Uint8 g, Uint8 b) {
    SDL_SetTextureColorMod(texture, r, g, b);
  }

  void ModAlpha(Uint8 a) { SDL_SetTextureAlphaMod(texture, a); }

  void Render(int x, int y, SDL_Rect *clip = NULL) {
    // set render space on screen
    // sprite will be stretched to match
    SDL_Rect renderDest = {x, y, width * scale, height * scale};

    // give dest rect the dimensions of the src rect
    if (clip != NULL) {
      renderDest.w = clip->w * scale;
      renderDest.h = clip->h * scale;
    }

    // render to screen
    // pass in clip as src rect
    SDL_RenderCopy(renderer, texture, clip, &renderDest);
  }

  void RenderRotated(int x, int y, SDL_Rect *clip, double angle,
                     SDL_Point *center, SDL_RendererFlip flip) {
    SDL_Rect renderDest = {x, y, width * scale, height * scale};

    // careful! this isn't given a value by default
    if (clip != NULL) {
      renderDest.w = clip->w * scale;
      renderDest.h = clip->h * scale;
    }

    // pass sprite through rotation
    SDL_RenderCopyEx(renderer, texture, clip, &renderDest, angle, center, flip);
  }

  void RenderFill() {
    // stretch to fill screen
    SDL_Rect renderDest = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

    SDL_RenderCopy(renderer, texture, NULL, &renderDest);
  }

  int GetWidth() { return width; }

  int GetHeight() { return height; }

  void SetScale(int nScale) { scale = nScale; }

private:
  SDL_Texture *texture;

  int width;
  int height;
  int scale;
};

auto lastUpdateTime = std::chrono::high_resolution_clock::now();
float dt = 0;

class LSprite {
public:
  LSprite(LTexture *spriteSheet, SDL_Rect *spriteClips, int nFrames) {
    this->spriteSheet = spriteSheet;
    this->spriteClips = spriteClips;

    this->nFrames = nFrames;

    currentFrame = 0;
    fTimer = 0;
    fps = 8;
  }

  void Render(int x, int y) {
    fTimer += dt;

    if (fTimer > (1.0 / fps)) {
      if (currentFrame + 1 == nFrames)
        currentFrame = 0;
      else
        currentFrame++;

      fTimer = 0;
    }

    // draw normal
    spriteSheet->Render(x, y, &spriteClips[currentFrame]);

    // draw rotated!
    // spriteSheet->RenderRotated(x, y, &spriteClips[currentFrame], 90, NULL,
    // SDL_FLIP_NONE);
  }

private:
  LTexture *spriteSheet;
  SDL_Rect *spriteClips;

  int currentFrame;
  int nFrames;
  int fps;
  float fTimer;
};

LTexture tBackground;

// character sprite
LTexture tSpriteSheet;
SDL_Rect spriteClips[] = {
    // frame 1
    {0, 0, 16, 16},
    // frame 2
    {0, 16, 16, 16}};

LSprite characterSprite(&tSpriteSheet, spriteClips, 2);

bool LoadMedia() {
  bool success = true;

  if (!tBackground.LoadFromFile("../assets/grass.png")) {
    printf("Could not load image: %s\n", SDL_GetError());
    success = false;
  }

  // this makes grass red
  // tBackground.ModColor(255, 0, 255);

  if (!tSpriteSheet.LoadFromFile("../assets/ness.png")) {
    printf("Could not load image: %s\n", SDL_GetError());
    success = false;
  }

  // this makes the char abt. half as opaque
  // tSpriteSheet.SetBlendMode(SDL_BLENDMODE_BLEND);
  // tSpriteSheet.ModAlpha(125);

  spriteClips[0] = {0, 0, 16, 16};

  spriteClips[1] = {0, 16, 16, 16};

  return success;
}

void Close() {
  // free loaded images
  tSpriteSheet.Free();
  tBackground.Free();

  // free window, renderer mem
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  renderer = NULL;
  window = NULL;

  // terminate sdl subsystems
  IMG_Quit();
  SDL_Quit();
}

// limits fps; adapt to set to a framerate
float targetFps = 120;

int charPosX = 0;
int charPosY = 0;
int charSpeed = 5;

int main(int argc, char *argv[]) {
  if (!Init())
    return 1;

  if (!LoadMedia())
    return 1;

  // window loop
  SDL_Event e;
  bool quit = false;
  while (!quit) {
    // get curr.time for dt calculation
    auto currentTime = std::chrono::high_resolution_clock::now();

    // compute dt
    dt = std::chrono::duration<float, std::chrono::seconds::period>(
             currentTime - lastUpdateTime)
             .count();

    // if dt does not exceed frame duration, move on
    if (dt < 1 / targetFps)
      continue;

    // poll returns 0 when no events, only run loop if events in it
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        quit = true;
      }

      // keydown event
      else if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE:
          quit = true;
          break;
        case SDLK_UP:
          KEYS[UP] = true;
          break;
        case SDLK_DOWN:
          KEYS[DOWN] = true;
          break;
        case SDLK_LEFT:
          KEYS[LEFT] = true;
          break;
        case SDLK_RIGHT:
          KEYS[RIGHT] = true;
          break;
        }
      }

      // keydown event
      else if (e.type == SDL_KEYUP) {
        switch (e.key.keysym.sym) {
          break;
        case SDLK_UP:
          KEYS[UP] = false;
          break;
        case SDLK_DOWN:
          KEYS[DOWN] = false;
          break;
        case SDLK_LEFT:
          KEYS[LEFT] = false;
          break;
        case SDLK_RIGHT:
          KEYS[RIGHT] = false;
          break;
        }
      }
    }

    // update player pos
    if (KEYS[UP])
      charPosY -= charSpeed;

    if (KEYS[DOWN])
      charPosY += charSpeed;

    if (KEYS[LEFT])
      charPosX -= charSpeed;

    if (KEYS[RIGHT])
      charPosX += charSpeed;

    // clear screen
    SDL_RenderClear(renderer);

    // draw
    tBackground.RenderFill();
    characterSprite.Render(charPosX, charPosY);

    // update screen
    SDL_RenderPresent(renderer);

    // update last time
    lastUpdateTime = currentTime;
  }

  return 1;
}
