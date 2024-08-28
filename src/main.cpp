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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

const int SCREEN_WIDTH = 512;
const int SCREEN_HEIGHT = 448;

SDL_Window *window = NULL; // render target
SDL_Surface *screenSurface = NULL; // screen surface
SDL_Renderer *renderer = NULL; // work with textures; hardware blitting
SDL_Rect screenRect = {
  0, 
  0, 
  SCREEN_WIDTH, 
  SCREEN_HEIGHT
};


bool Init() {
  // start sdl
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL init failed: %s\n", SDL_GetError());
    return false;
  }

  // make window
  window = SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
  if (window == NULL) {
    printf("Window creation failed: %s\n", SDL_GetError());
    return false;
  }

  // make renderer for window
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL) {
    printf("Could not create renderer :%s\n", SDL_GetError());
    return false;
  }

  // adjust renderer color used for various operations (?)
  SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0xFF, 0xFF);

  // start up sdl image loader
  int imageFlags = IMG_INIT_PNG; // this bitmask should result in a 1
  int initResult = IMG_Init(imageFlags) & imageFlags;
  if (!initResult) {
    printf("Could not load SDL image: %s\n", SDL_GetError());
    return false;
  }

  // get window surface
  screenSurface = SDL_GetWindowSurface(window);

  return true;
}

// texture wrapper
class LTexture {
public:
  LTexture(){
    texture = NULL;
    width = 0;
    height = 0;
  }

  ~LTexture(){
    Free();  
  }

  bool LoadFromFile(const char* path){
    // remove pre-existing texture
    Free();

    // load new one
    SDL_Surface* lSurf = IMG_Load(path);

    if (lSurf == NULL){
      printf("Unable to load image: %s\n", SDL_GetError());
      return false;
    }

    // set color key to black
    SDL_SetColorKey(lSurf, SDL_TRUE, SDL_MapRGB(lSurf->format, 0x00, 0x00, 0x00));
   
    // make texture w/color key
    SDL_Texture *nTexture = SDL_CreateTextureFromSurface(renderer, lSurf);

    if (nTexture == NULL){
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

  void Free(){
    if (texture != NULL){
      SDL_DestroyTexture(texture);
      texture = NULL;
      width = 0;
      height = 0;
    }
  }

  void Render(int x, int y){
    // set render space on screen
    // sprite will be stretched to match
    SDL_Rect renderDest = {
      x,
      y,
      width * scale,
      height * scale 
    };

    // render to screen
    SDL_RenderCopy(renderer, texture, NULL, &renderDest);
  }

  int GetWidth(){
    return width;
  }

  int GetHeight(){
    return height;
  }

  void SetScale(int nScale){
    scale = nScale;
  }

private:
  SDL_Texture* texture;

  int width;
  int height;
  int scale;
};

LTexture tCharacter;
LTexture tBackground;

bool LoadMedia(){
  bool success = true;
  
  if (!tCharacter.LoadFromFile("../assets/ness.png")){
    printf("Could not load image: %s\n", SDL_GetError());
    success = false;
  }

  tCharacter.SetScale(5);

  return success;
}

void Close() {
  // free loaded images
  tCharacter.Free();
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

int main(int argc, char *argv[]) {
  if (!Init())
    return 1;

  if (!LoadMedia())
    return 1;

  // window loop
  SDL_Event e;
  bool quit = false;
  while (!quit) {
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
          break;
        case SDLK_DOWN:
          break;
        case SDLK_LEFT:
          break;
        case SDLK_RIGHT:
          break;
        default:
          break;
        }
      }
    }

    // clear screen
    SDL_RenderClear(renderer);
    
    // draw
    tCharacter.Render(0, 0);

    // update screen
    SDL_RenderPresent(renderer);
  }

  return 1;
}
