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
#include <SDL_clipboard.h>
#include <SDL_rwops.h>
#include <SDL_scancode.h>
#include <SDL_stdinc.h>
#include <chrono>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <vector>

const int SCREEN_WIDTH = 900;
const int SCREEN_HEIGHT = 900;

const int LEVEL_WIDTH = 3 * SCREEN_WIDTH;
const int LEVEL_HEIGHT = 3 * SCREEN_HEIGHT;

const int GLOB_SCALE = 8;
const int GLOB_FONTSIZE = 32;

const int KEY_COUNT = 4;

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

SDL_Rect cam = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

SDL_Color textColor = {255, 255, 255, 255};
std::string inputText = "Input Text";

// gamesave
const int TOTAL_DATA = 10;
Sint32 saveData[TOTAL_DATA];

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
    scale = 1;
    renderDest = {0, 0, 0, 0};
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
    renderDest = {x, y, width, height};

    // give dest rect the dimensions of the src rect
    if (clip != NULL) {
      renderDest.w = clip->w;
      renderDest.h = clip->h;
    }

    renderDest.w *= scale;
    renderDest.h *= scale;

    // render to screen
    // pass in clip as src rect
    SDL_RenderCopy(renderer, texture, clip, &renderDest);
  }

  void RenderRotated(int x, int y, SDL_Rect *clip, double angle,
                     SDL_Point *center, SDL_RendererFlip flip) {
    renderDest = {x, y, width, height};

    // careful! this isn't given a value by default
    if (clip != NULL) {
      renderDest.w = clip->w;
      renderDest.h = clip->h;
    }

    renderDest.w *= scale;
    renderDest.h *= scale;

    // pass sprite through rotation
    SDL_RenderCopyEx(renderer, texture, clip, &renderDest, angle, center, flip);
  }

  void RenderFill() {
    // stretch to fill screen
    renderDest = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

    SDL_RenderCopy(renderer, texture, NULL, &renderDest);
  }

  void RenderIgnoreScale(int x, int y, int w, int h, SDL_Rect *clip = NULL) {
    // renders ignoring set scale; useful for ui that needs specific dimensions
    renderDest = {x, y, w, h};

    // not sure what happens if clip wh don't match given wh; does it stretch?
    // yup, it does stretch!
    SDL_RenderCopy(renderer, texture, clip, &renderDest);
  }

  // entire texture w + h are given if render dest dimensions match
  // but if render dest dimensions are different (we do this when rendering
  // frames, for ex.) we return the modified w, h instead
  // also if we haven't rendered this yet (renderDest w/h are 0) just use
  // regular w/h too

  int GetWidth() {
    if (renderDest.w == width || renderDest.w == 0) {
      return width * scale;
    }

    return renderDest.w;
  }

  int GetHeight() {
    if (renderDest.h == height || renderDest.h == 0) {
      return height * scale;
    }

    return renderDest.h;
  }

  void SetScale(int nScale) { scale = nScale; }

private:
  SDL_Texture *texture;
  SDL_Rect renderDest;

  int width;
  int height;
  int scale;
};

LTexture tBackground;
LTexture tPrompt;
LTexture tTimer;
LTexture tInput;
LTexture tSpriteSheet;
LTexture tButton;
LTexture tLavaThingSpriteSheet;
LTexture tBrick;

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

  bool GetMovedFrame() { return movedFrame; }

  int GetFPS() { return this->fps; }

  int GetWidth() { return spriteSheet->GetWidth(); }

  int GetHeight() { return spriteSheet->GetHeight(); }

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
    movedFrame = false;

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
  bool movedFrame;
  float fTimer;
};

SDL_Rect charSpriteClips[] = {{0, 0, 16, 16}, {0, 16, 16, 16}};

LSprite charSprite(&tSpriteSheet, charSpriteClips, 2);

SDL_Rect lavaThingSpriteClips[] = {{0, 0, 8, 8}, {0, 8, 8, 8}};

SDL_Rect buttonSpriteClips[] = {{0, 0, 8, 8}, {0, 8, 8, 8}, {0, 16, 8, 8}};

class LButton {
public:
  static const int BUTTON_WIDTH = 89;
  static const int BUTTON_HEIGHT = 89;

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

bool CheckCollision(SDL_Rect a, SDL_Rect b) {
  // sides of both rects
  int leftA, leftB;
  int rightA, rightB;
  int topA, topB;
  int bottomA, bottomB;

  // calculate sides of a
  leftA = a.x;
  rightA = a.x + a.w;
  topA = a.y;
  bottomA = a.y + a.h;

  // calculate sides of b
  leftB = b.x;
  rightB = b.x + b.w;
  topB = b.y;
  bottomB = b.y + b.h;

  // if any sides from a are outside b
  if (bottomA <= topB) {
    return false;
  }

  if (topA >= bottomB) {
    return false;
  }

  if (rightA <= leftB) {
    return false;
  }

  if (leftA >= rightB) {
    return false;
  }

  // if no sides of a are outside b
  return true;
}

class Tile {
public:
  static const int TILE_WIDTH = 100;
  static const int TILE_HEIGHT = 100;

  Tile() {
    collider.x = 0;
    collider.y = 0;
    collider.w = TILE_WIDTH;
    collider.h = TILE_HEIGHT;

    posX = 0;
    posY = 0;

    texture = &tBrick;
  }

  SDL_Rect *GetCollider() { return &collider; }

  void SetPosition(int x, int y, int camX = 0, int camY = 0) {
    posX = x;
    posY = y;

    collider.x = x - camX;
    collider.y = y - camY;
  }

  void ApplyCameraOffset(int camX, int camY) {
    // tiles are static by nature (pos stays same), but their colliders should
    // have the cam. offset applied to them
    collider.x = posX - camX;
    collider.y = posY - camY;
  }

  void Render(int camX, int camY) {
    texture->RenderIgnoreScale(posX - camX, posY - camY, collider.w,
                               collider.h);
  }

private:
  SDL_Rect collider;
  LTexture *texture;

  int posX, posY;
};

const int TILE_COUNT = 5;
std::vector<Tile> tiles;

class Player {
public:
  static const int PLAYER_VEL = 5;

  LSprite sprite;

  // sprite doesn't have a default constructor; must explicitly initialize it
  // using this syntax
  Player() : sprite(&tSpriteSheet, charSpriteClips, 2) {
    posX = 0;
    posY = 0;

    velX = 0;
    velY = 0;

    // will take the dimensions of curr. frame, for better or worse
    collider.w = 0;
    collider.h = 0;
  }

  SDL_Rect *GetCollider() { return &this->collider; }

  void HandleEvent(SDL_Event &e) {
    // when key is pressed, update velocity to match direction
    if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
      switch (e.key.keysym.sym) {
      case SDLK_UP:
        velY -= PLAYER_VEL;
        break;
      case SDLK_DOWN:
        velY += PLAYER_VEL;
        break;
      case SDLK_LEFT:
        velX -= PLAYER_VEL;
        break;
      case SDLK_RIGHT:
        velX += PLAYER_VEL;
        break;
      }
    }

    // when key released, undo vel change not by setting to 0, but by
    // subtracting what the downpress did very logical, it doesn't mess with
    // any other vel changes we made!
    else if (e.type == SDL_KEYUP && e.key.repeat == 0) {
      switch (e.key.keysym.sym) {
      case SDLK_UP:
        velY += PLAYER_VEL;
        break;
      case SDLK_DOWN:
        velY -= PLAYER_VEL;
        break;
      case SDLK_LEFT:
        velX += PLAYER_VEL;
        break;
      case SDLK_RIGHT:
        velX -= PLAYER_VEL;
        break;
      }
    }
  }

  void SetPosition(int x, int y) {
    posX = x;
    posY = y;
  }

  bool CheckTileCollisions(std::vector<Tile> &tiles) {
    // this seems a bit expensive for large sets :/
    for (int i = 0; i < tiles.size(); ++i) {
      if (CheckCollision(collider, *tiles[i].GetCollider())) {
        return true;
      }
    }

    return false;
  }

  int GetPosX() { return posX; }

  int GetPosY() { return posY; }

  void Move(std::vector<Tile> &tiles, int camX, int camY) {
    // update collider with position
    // this fixes clipping (how???)
    posX += velX;
    collider.x = posX - camX;
    collider.w = sprite.GetWidth();

    // reverse vel if hit bounds
    // account for the fact that pos is in topleft for 2nd part of ||
    // dont move if colliding
    if (posX < 0 || posX + sprite.GetWidth() > LEVEL_WIDTH ||
        CheckTileCollisions(tiles)) {
      posX -= velX;
      collider.x = posX;
    }

    posY += velY;
    collider.y = posY - camY;
    collider.h = sprite.GetHeight();

    if (posY < 0 || posY + sprite.GetHeight() > LEVEL_HEIGHT ||
        CheckTileCollisions(tiles)) {
      posY -= velY;
      collider.y = posY;
    }
  }

  void Render(int camX, int camY) {
    // set sprite fps depending on keydown state
    if (KEYS[UP] || KEYS[DOWN] || KEYS[LEFT] || KEYS[RIGHT]) {
      sprite.SetFPS(4);
    }

    else {
      sprite.SetFPS(0);
    }

    sprite.Render(posX - camX, posY - camY);
  }

  void PlaySound() {
    if (sprite.GetMovedFrame()) {
      Mix_PlayChannel(-1, step, 0);
    }
  }

private:
  int posX, posY;
  int velX, velY;
  SDL_Rect collider;
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
      SDL_RENDERER_ACCELERATED); // we can | SDL_RENDERER_PRESENTVSYNC to
                                 // enable that, but don't; already have
                                 // target fps system

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
  sampleButton.SetPosition((SCREEN_WIDTH / 2) - (LButton::BUTTON_WIDTH / 2),
                           (SCREEN_HEIGHT / 2) - (LButton::BUTTON_HEIGHT / 2));

  // instantiate and position tiles

  for (int i = 0; i < TILE_COUNT; ++i) {
    tiles.push_back(Tile());
  }

  tiles[0].SetPosition(0, 0);
  tiles[1].SetPosition(Tile::TILE_WIDTH, 0);
  tiles[2].SetPosition(Tile::TILE_WIDTH * 2, 0);
  tiles[3].SetPosition(Tile::TILE_WIDTH * 3, 0);
  tiles[4].SetPosition(Tile::TILE_WIDTH * 4, 0);

  // position player
  player.SetPosition(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

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
  tInput.LoadFromRenderedText(inputText.c_str(), textColor);
  tPrompt.LoadFromRenderedText("Sample text", textColor);
  tTimer.LoadFromRenderedText("Press ENTER to track time!", textColor);

  // mk statusbar bg from text surf size
  statusBarBG = {0, SCREEN_HEIGHT - tPrompt.GetHeight() - 10, SCREEN_WIDTH,
                 tPrompt.GetHeight() + 10};

  // bg
  if (!tBackground.LoadFromFile("../assets/grass_large.png")) {
    printf("Could not load image: %s\n", SDL_GetError());
    success = false;
  }

  // brick
  if (!tBrick.LoadFromFile("../assets/brick.png")) {
    printf("Could not load image: %s\n", SDL_GetError());
    success = false;
  }

  // char sprite
  if (!tSpriteSheet.LoadFromFile("../assets/ness.png")) {
    printf("Could not load image: %s\n", SDL_GetError());
    success = false;
  }

  tSpriteSheet.SetScale(GLOB_SCALE);

  // lava thing sprite
  if (!tLavaThingSpriteSheet.LoadFromFile("../assets/lavathing.png")) {
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

  // savefile, open in binary read mode
  SDL_RWops *file = SDL_RWFromFile("../assets/save.bin", "r+b");

  // handle nonexistent file
  if (file == NULL) {
    printf("Unable to open savefile: %s\n", SDL_GetError());

    // make new file by opening in binary write mode
    file = SDL_RWFromFile("../assets/save.bin", "w+b");

    if (file != NULL) {
      printf("New save created!\n");

      // init data
      for (int i = 0; i < TOTAL_DATA; ++i) {
        saveData[i] = 0;

        // write to file
        // args: file obj, address of src. obj, size of src. obj, amt. of
        // objects we're writing why don't we just write TOTAL_DATA objs?
        SDL_RWwrite(file, &saveData[i], sizeof(Sint32), 1);
      }

      // close file
      SDL_RWclose(file);
    }

    else {
      printf("Unable to create save: %s\n", SDL_GetError());
      success = false;
    }
  }

  // if file did exist, read its data instead
  else {
    printf("Reading savefile...\n");

    for (int i = 0; i < TOTAL_DATA; ++i) {
      SDL_RWread(file, &saveData[i], sizeof(Sint32), 1);
    }

    SDL_RWclose(file);
  }

  return success;
}

void Close() {
  // update savefile
  SDL_RWops *file = SDL_RWFromFile("../assets/save.bin", "w+b");

  if (file != NULL){
    printf("Saving data...\n");

    for (int i = 0; i < TOTAL_DATA; ++i){
      SDL_RWwrite(file, &saveData[i], sizeof(Sint32), 1);
    }
    
    SDL_RWclose(file);
  }

  else{
    printf("Unable to save file: %s\n", SDL_GetError());
  }

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

  // start text input, seems to be on by default?
  // SDL_StartTextInput();

  // need to call this when we want game to stop taking text input
  SDL_StopTextInput();

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

    // update input text when it changes only
    bool renderInputText = false;

    // if dt does not exceed frame duration, move on
    if (dt < 1 / targetFps)
      continue;

    // poll returns 0 when no events, only run loop if events in it
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        quit = true;
      }

      else if (e.type == SDL_KEYDOWN) {
        // music controls
        if (e.key.keysym.sym == SDLK_p && e.key.repeat == 0 &&
            !SDL_IsTextInputActive()) {
          if (Mix_PlayingMusic() == 0) {
            Mix_PlayMusic(music, -1);
          }

          else if (Mix_PausedMusic() == 0) {
            Mix_PauseMusic();
          }

          else if (Mix_PausedMusic() == 1) {
            Mix_ResumeMusic();
          }
        }

        // input special key handling

        // backspace
        else if (e.key.keysym.sym == SDLK_BACKSPACE && inputText.length() > 0) {
          inputText.pop_back();
          renderInputText = true;
        }

        // copy
        // getmodstate returns or'd combo of keyboard states, kmodctrl denotes
        // ctrl held down i think check what that bitwise looks like on paper
        else if (e.key.keysym.sym == SDLK_c && SDL_GetModState() & KMOD_CTRL) {
          SDL_SetClipboardText(inputText.c_str());
        }

        // paste
        else if (e.key.keysym.sym == SDLK_v && SDL_GetModState() & KMOD_CTRL) {
          // get text from clipboard into buffer, put it into input and then
          // clear
          char *tempText = SDL_GetClipboardText();
          inputText = tempText;

          // what is the difference between this and free() ?
          SDL_free(tempText);

          renderInputText = true;
        }
      }

      else if (e.type == SDL_TEXTINPUT) {
        // make sure we aren't copying or pasting
        bool pressingC = e.text.text[0] == 'c' || e.text.text[0] == 'C';
        bool pressingV = e.text.text[0] == 'v' || e.text.text[0] == 'V';

        if (!(SDL_GetModState() & KMOD_CTRL && (pressingC || pressingV))) {
          // append char to input text
          inputText += e.text.text;
          renderInputText = true;
        }
      }

      // render input text if needed
      if (renderInputText) {
        // make sure text isn't empty
        if (inputText != "") {
          tInput.LoadFromRenderedText(inputText.c_str(), textColor);
        }

        // if so, just render whitespace
        else {
          tInput.LoadFromRenderedText(" ", textColor);
        }
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

      // button event
      // sampleButton.HandleEvent(&e);

      // player event
      player.HandleEvent(e);
    }

    // clear screen
    SDL_RenderClear(renderer);

    // esc check
    if (KEYS[EXIT]) {
      quit = true;
    }

    // render bg
    tBackground.Render(0, 0, &cam);

    // render tiles
    for (int i = 0; i < 5; ++i) {
      tiles[i].ApplyCameraOffset(cam.x, cam.y);
      tiles[i].Render(cam.x, cam.y);
    }

    // button
    // sampleButton.Render();

    // update player
    player.Move(tiles, cam.x, cam.y);
    player.Render(cam.x, cam.y);
    player.PlaySound();

    // center camera over player
    cam.x =
        (player.GetPosX() + player.sprite.GetWidth() / 2) - SCREEN_WIDTH / 2;
    cam.y =
        (player.GetPosY() + player.sprite.GetHeight() / 2) - SCREEN_HEIGHT / 2;

    // make sure cam doesn't leave bounds, causes weird stretch
    // we need to ensure level dimensions match level texture's!
    if (cam.x < 0) {
      cam.x = 0;
    }

    if (cam.x > LEVEL_WIDTH - cam.w) {
      cam.x = LEVEL_WIDTH - cam.w;
    }

    if (cam.y < 0) {
      cam.y = 0;
    }

    if (cam.y > LEVEL_HEIGHT - cam.h) {
      cam.y = LEVEL_HEIGHT - cam.h;
    }

    // avg. fps = frames / time
    float avgFPS = countedFrames / (fpsTimer.GetTicks() / 1000.0f);

    // if fps is too large, just display it as 0
    if (avgFPS > 2000000) {
      avgFPS = 0;
    }

    // timer text with background
    SDL_RenderFillRect(renderer, &statusBarBG);

    // render info text
    // timeText.str("");
    // timeText << "FPS: " << (int)avgFPS << ", Music: "
    //          << (Mix_PausedMusic() == 1 || Mix_PlayingMusic() == 0 ?
    //          "Stopped"
    //                                                                :
    //                                                                "Playing");

    // if (!tTimer.LoadFromRenderedText(timeText.str().c_str(), textColor)) {
    //   printf("Unable to update text texture: %s\n", SDL_GetError());
    // }

    // tTimer.Render(0, statusBarBG.y + (statusBarBG.h - tPrompt.GetHeight()) /
    // 2);

    // render input text
    tInput.Render(0, statusBarBG.y + (statusBarBG.h - tPrompt.GetHeight()) / 2);

    // update screen
    SDL_RenderPresent(renderer);

    // update last time
    lastUpdateTime = currentTime;

    // increment counted frames
    countedFrames++;
  }

  Close();

  return 1;
}
