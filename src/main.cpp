#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_video.h>
#include <SDL_scancode.h>
#include <chrono>
#include <sstream>
#include <stdio.h>
#include <string.h>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 800;

const int GLOB_SCALE = 8;
const int GLOB_FONTSIZE = 32;

const int KEY_COUNT = 4;

const int BUTTON_WIDTH = 89;
const int BUTTON_HEIGHT = 89;

SDL_Window *window = NULL;
SDL_Surface *screenSurface = NULL;
SDL_Renderer *renderer = NULL;
SDL_Rect screenRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

TTF_Font *gFont = NULL;
SDL_Rect statusBarBG;

typedef enum Inputs { UP, DOWN, LEFT, RIGHT, PAUSE, EXIT, TOTAL_INPUTS } Inputs;
bool KEYS[TOTAL_INPUTS];

auto lastUpdateTime = std::chrono::high_resolution_clock::now();

float targetFps = 120;
float dt = 0;

Mix_Music *music = NULL;
Mix_Chunk *step = NULL;

std::stringstream timeText;

typedef enum LButtonState {
  BUTTON_STATE_YELLOW,
  BUTTON_STATE_RED,
  BUTTON_STATE_GREEN
} LButtonState;

class LTexture {
public:
  LTexture() {
    texture = NULL;
    width = 0;
    height = 0;
    scale = 0;
  }

  ~LTexture() { Free(); }

// anything inside #if will be ignored by compiler if SDL_TTF ... macro is not
// defined
#if defined(SDL_TTF_MAJOR_VERSION)

  bool LoadFromRenderedText(const char *text, SDL_Color tColor) {
    // free old texture
    Free();

    // render text
    SDL_Surface *tSurf = TTF_RenderText_Solid(gFont, text, tColor);
    if (tSurf == NULL) {
      printf("Unable to render text: %s\n", SDL_GetError());
      return false;
    }

    // create texture
    texture = SDL_CreateTextureFromSurface(renderer, tSurf);
    if (tSurf == NULL) {
      printf("Unable to make texture from text: %s\n", SDL_GetError());
      return false;
    }

    // because we're already giving fontsize, it's unnecessary to set scale too
    SetScale(1);

    width = tSurf->w;
    height = tSurf->h;

    SDL_FreeSurface(tSurf);

    return true;
  }

#endif

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
    SDL_SetColorKey(lSurf, SDL_TRUE, SDL_MapRGB(lSurf->format, 0, 0, 0));

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

  void RenderIgnoreScale(int x, int y, int w, int h, SDL_Rect *clip = NULL) {
    // renders ignoring set scale; useful for ui that needs specific dimensions
    SDL_Rect renderDest = {x, y, w, h};

    // not sure what happens if clip wh don't match given wh; does it stretch?
    // yup, it does stretch!
    SDL_RenderCopy(renderer, texture, clip, &renderDest);
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

LTexture tBackground;
LTexture tPrompt;
LTexture tTimer;
LTexture tSpriteSheet;
LTexture tButton;
LTexture tLavaThingSpriteSheet;

class LSprite {
public:
  LSprite(LTexture *spriteSheet, SDL_Rect *spriteClips, int nFrames) {
    this->spriteSheet = spriteSheet;
    this->spriteClips = spriteClips;

    this->nFrames = nFrames;

    currentFrame = 0;
    fTimer = 0;
    fps = 4;
  }

  float GetFrameTimer() { return this->fTimer; }

  int GetFPS() { return this->fps; }

  void SetFPS(int fps) {
    if (fps < 0) {
      printf("Could not set FPS! Out of bounds.\n");
      return;
    }

    this->fps = fps;
  }

  void SetFrame(int f) {
    if (f < 0 || f >= nFrames) {
      printf("Could not set frame! Out of bounds.\n");
      return;
    }

    this->currentFrame = f;
  }

  bool Render(int x, int y) {
    bool movedFrame = false;

    if (fps > 0) {
      fTimer += dt;

      if (fTimer > (1.0 / fps)) {
        movedFrame = true;

        if (currentFrame + 1 == nFrames) {
          currentFrame = 0;
        } else {
          currentFrame++;
        }

        fTimer = 0;
      }
    }

    // draw
    spriteSheet->Render(x, y, &spriteClips[currentFrame]);

    return movedFrame;
  }

private:
  LTexture *spriteSheet;
  SDL_Rect *spriteClips;

  int currentFrame;
  int nFrames;
  int fps;
  float fTimer;
};

SDL_Rect charSpriteClips[] = {{0, 0, 16, 16}, {0, 16, 16, 16}};

LSprite charSprite(&tSpriteSheet, charSpriteClips, 2);

SDL_Rect lavaThingSpriteClips[] = {{0, 0, 8, 8}, {0, 8, 8, 8}};

SDL_Rect buttonSpriteClips[] = {{0, 0, 8, 8}, {0, 8, 8, 8}, {0, 16, 8, 8}};

class LButton {
public:
  LButton() {
    mPosition.x = 0;
    mPosition.y = 0;
    mState = BUTTON_STATE_RED;
  }

  void SetPosition(int x, int y) {
    mPosition.x = x;
    mPosition.y = y;
  }

  void HandleEvent(SDL_Event *e) {
    // enter this if any mouse events happen
    if (e->type == SDL_MOUSEMOTION || e->type == SDL_MOUSEBUTTONDOWN ||
        e->type == SDL_MOUSEBUTTONUP) {
      // get mouse pos
      int x, y;
      SDL_GetMouseState(&x, &y);

      // check if mouse within button
      bool inside = true;

      if (x < mPosition.x) {
        inside = false;
      }

      else if (x > mPosition.x + BUTTON_WIDTH) {
        inside = false;
      }

      else if (y < mPosition.y) {
        inside = false;
      }

      else if (y > mPosition.y + BUTTON_HEIGHT) {
        inside = false;
      }

      // change button state depending on mouse event
      if (!inside) {
        mState = BUTTON_STATE_RED;
      }

      else {
        switch (e->type) {
        // case SDL_MOUSEMOTION:
        //   mState = BUTTON_STATE_YELLOW;
        // break;
        case SDL_MOUSEBUTTONDOWN:
          mState = BUTTON_STATE_GREEN;
          break;
        case SDL_MOUSEBUTTONUP:
          mState = BUTTON_STATE_RED;
          break;
        }
      }
    }
  }

  void Render() {
    // render button texture
    // don't use sprite class because state is managed by instance
    tButton.RenderIgnoreScale(mPosition.x, mPosition.y, BUTTON_WIDTH,
                              BUTTON_HEIGHT, &buttonSpriteClips[mState]);
  }

private:
  SDL_Point mPosition;
  LButtonState mState;
};

LButton sampleButton;

class LTimer {
public:
  LTimer() {
    startTicks = 0;
    pausedTicks = 0;

    paused = false;
    started = false;
  }

  void Start() {
    // start timer

    started = true;
    paused = false;

    startTicks = SDL_GetTicks();
    pausedTicks = 0;
  }

  void Stop() {
    // completely stop timer w/o intent of resuming

    started = false;
    paused = false;

    startTicks = 0;
    pausedTicks = 0;
  }

  void Pause() {
    // can only pause if already running and not paused
    if (started && !paused) {
      paused = true;

      // get time kept before pause
      pausedTicks = SDL_GetTicks() - startTicks;

      // reset counted runtime
      startTicks = 0;
    }
  }

  void Unpause() {
    if (started && paused) {
      paused = false;

      // get time kept during pause
      startTicks = SDL_GetTicks() - pausedTicks;

      // reset paused ticks
      pausedTicks = 0;
    }
  }

  // get timer curr. time
  Uint32 GetTicks() {
    // actual kept time
    Uint32 time = 0;

    if (started) {
      // if paused, return time kept before pause
      if (paused) {
        time = pausedTicks;
      }

      // if not, return time kept since last start/unpause
      else {
        time = SDL_GetTicks() - startTicks;
      }
    }

    return time;
  }

  // check status of timer
  bool IsStarted() { return started; }

  bool IsPaused() {
    // paused is different from stopped completely
    return paused && paused;
  }

private:
  Uint32 startTicks;  // time when timer started
  Uint32 pausedTicks; // ticks when timer paused

  bool paused;
  bool started;
};

LTimer fpsTimer;

int countedFrames = 0;

class Player {
public:
  static const int DOT_WIDTH = 20;
  static const int DOT_HEIGHT = 20;
  static const int DOT_VEL = 10;

  LSprite sprite;

  // sprite doesn't have a default constructor; must explicitly initialize it using this syntax
  Player() : sprite(&tLavaThingSpriteSheet, lavaThingSpriteClips, 2){
    posX = 0;
    posY = 0;

    velX = 0;
    velY = 0;
  }

  void HandleEvent(SDL_Event &e) {
    // when key is pressed, update velocity to match direction
    if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
      switch (e.key.keysym.sym) {
      case SDLK_UP:
        velY -= DOT_VEL;
        break;
      case SDLK_DOWN:
        velY += DOT_VEL;
        break;
      case SDLK_LEFT:
        velX -= DOT_VEL;
        break;
      case SDLK_RIGHT:
        velX += DOT_VEL;
        break;
      }
    }

    // when key released, undo vel change not by setting to 0, but by
    // subtracting what the downpress did very logical, it doesn't mess with any
    // other vel changes we made!
    else if (e.type == SDL_KEYUP && e.key.repeat == 0) {
      switch (e.key.keysym.sym) {
      case SDLK_UP:
        velY += DOT_VEL;
        break;
      case SDLK_DOWN:
        velY -= DOT_VEL;
        break;
      case SDLK_LEFT:
        velX += DOT_VEL;
        break;
      case SDLK_RIGHT:
        velX -= DOT_VEL;
        break;
      }
    }
  }

  void Move() {
    posX += velX;

    // reverse vel if hit bounds
    // account for the fact that pos is in topleft for 2nd part of ||
    if (posX < 0 || (posX + DOT_WIDTH > SCREEN_WIDTH)) {
      posX -= velX;
    }

    posY += velY;

    if (posY < 0 || (posY + DOT_HEIGHT > SCREEN_HEIGHT)) {
      posY -= velY;
    }
  }

  void Render() {
    sprite.Render(posX, posY); 
  }

private:
  int posX, posY;
  int velX, velY;
};

Player player;

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

  // adjust renderer color used
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

  // start up sdl image loader
  int imageFlags = IMG_INIT_PNG; // this bitmask should result in a 1
  int initResult = IMG_Init(imageFlags) & imageFlags;
  if (!initResult) {
    printf("Could not load SDL image: %s\n", SDL_GetError());
    return false;
  }

  // start ttf loader
  if (TTF_Init() == -1) {
    printf("Could not load SDL ttf: %s\n", SDL_GetError());
    return false;
  }

  // start mixer
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
    printf("Could not load mixer: %s\n", SDL_GetError());
    return false;
  }

  // get window surface
  screenSurface = SDL_GetWindowSurface(window);

  // set keys
  for (int i = 0; i < KEY_COUNT; ++i)
    KEYS[i] = false;

  // position button
  sampleButton.SetPosition((SCREEN_WIDTH / 2) - (BUTTON_WIDTH / 2),
                           (SCREEN_HEIGHT / 2) - (BUTTON_HEIGHT / 2));

  // start the fps timer
  fpsTimer.Start();

  return true;
}

bool LoadMedia() {
  bool success = true;

  // font
  gFont = TTF_OpenFont("../assets/pixel-font.ttf", GLOB_FONTSIZE);
  if (gFont == NULL) {
    printf("Failed to load gFont: %s\n", SDL_GetError());
    success = false;
  }

  // text
  tPrompt.LoadFromRenderedText("Sample text", {255, 255, 255});
  tTimer.LoadFromRenderedText("Press ENTER to track time!", {255, 255, 255});

  // mk statusbar bg from text surf size
  statusBarBG = {0, SCREEN_HEIGHT - tPrompt.GetHeight() - 10, SCREEN_WIDTH,
                 tPrompt.GetHeight() + 10};

  // bg
  if (!tBackground.LoadFromFile("../assets/grass.png")) {
    printf("Could not load image: %s\n", SDL_GetError());
    success = false;
  }

  // char sprite
  if (!tSpriteSheet.LoadFromFile("../assets/ness.png")) {
    printf("Could not load image: %s\n", SDL_GetError());
    success = false;
  }

  // lava thing sprite
  if (!tLavaThingSpriteSheet.LoadFromFile("../assets/lavathing.png")){
    printf("Could not load image: %s\n", SDL_GetError());
    success = false;
  }

  // button sprite
  if (!tButton.LoadFromFile("../assets/button.png")) {
    printf("Could not load image: %s\n", SDL_GetError());
    success = false;
  }

  // sounds
  step = Mix_LoadWAV("../assets/step.wav");
  if (step == NULL) {
    printf("Failed to load SFX: %s\n", SDL_GetError());
    success = false;
  }

  // music
  music = Mix_LoadMUS("../assets/music.mp3");
  if (music == NULL) {
    printf("Failed to load music: %s\n", SDL_GetError());
    success = false;
  }

  return success;
}

void Close() {
  // free loaded images
  tSpriteSheet.Free();
  tBackground.Free();
  tSpriteSheet.Free();
  tButton.Free();

  // free sfx
  Mix_FreeChunk(step);

  // free music
  Mix_FreeMusic(music);

  // free window, renderer mem
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  renderer = NULL;
  window = NULL;

  // terminate sdl subsystems
  IMG_Quit();
  Mix_Quit();
  SDL_Quit();
}

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

      // pointer to array containing kb state
      // upd'd everytime PollEvent called, so we should put this in ev. loop
      const Uint8 *currentKeyStates = SDL_GetKeyboardState(NULL);

      // update input state using latter
      KEYS[UP] = currentKeyStates[SDL_SCANCODE_UP];
      KEYS[DOWN] = currentKeyStates[SDL_SCANCODE_DOWN];
      KEYS[LEFT] = currentKeyStates[SDL_SCANCODE_LEFT];
      KEYS[RIGHT] = currentKeyStates[SDL_SCANCODE_RIGHT];
      KEYS[PAUSE] = currentKeyStates[SDL_SCANCODE_P];
      KEYS[EXIT] = currentKeyStates[SDL_SCANCODE_ESCAPE];

      // mouse event
      sampleButton.HandleEvent(&e);

      // player event
      player.HandleEvent(e);
    }

    // clear screen
    SDL_RenderClear(renderer);

    // esc check
    if (KEYS[EXIT]) {
      quit = true;
    }

    // music control, is broken :/
    // if (KEYS[PAUSE]) {
    //   if (Mix_PausedMusic()) {
    //     Mix_ResumeMusic();
    //   } else {
    //     Mix_PauseMusic();
    //   }
    // }

    // render bg
    tBackground.RenderFill();

    // button
    sampleButton.Render();

    // update player
    player.Move();
    player.Render();
    
    // timer text with background
    SDL_RenderFillRect(renderer, &statusBarBG);

    // avg. fps = frames / time
    float avgFPS = countedFrames / (fpsTimer.GetTicks() / 1000.0f);

    // if fps is too large, just display it as 0
    if (avgFPS > 2000000) {
      avgFPS = 0;
    }

    // render fps info
    timeText.str("");
    timeText << "FPS: " << avgFPS;

    if (!tTimer.LoadFromRenderedText(timeText.str().c_str(),
                                     {255, 255, 255, 255})) {
      printf("Unable to update text texture: %s\n", SDL_GetError());
    }

    tTimer.Render(0, statusBarBG.y + (statusBarBG.h - tPrompt.GetHeight()) / 2);

    // play music on first free channel (-1)
    if (Mix_PlayingMusic() == 0) {
      Mix_PlayMusic(music, -1);
    }

    // update screen
    SDL_RenderPresent(renderer);

    // update last time
    lastUpdateTime = currentTime;

    // increment counted frames
    countedFrames++;
  }

  return 1;
}
