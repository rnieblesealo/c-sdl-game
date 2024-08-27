#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
#include <SDL_keycode.h>
#include <SDL_rect.h>
#include <SDL_render.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 256;

SDL_Window *window = NULL; // render target
SDL_Surface *screenSurface =
    NULL; // surface (2d image) contained by render target

enum KeyPressSurfaces {
  KEY_PRESS_SURFACE_DEFAULT,
  KEY_PRESS_SURFACE_UP,
  KEY_PRESS_SURFACE_DOWN,
  KEY_PRESS_SURFACE_LEFT,
  KEY_PRESS_SURFACE_RIGHT,
  KEY_PRESS_SURFACE_TOTAL
};

// renderers work with textures; they use hardware as opposed to blitting (?),
// thus more efficient!
SDL_Renderer *renderer = NULL;
SDL_Texture *images[KEY_PRESS_SURFACE_TOTAL];
SDL_Texture *image = NULL;

// geometry rendering!
SDL_Rect fillRect = {SCREEN_WIDTH / 4, SCREEN_HEIGHT / 4, SCREEN_WIDTH / 2,
                     SCREEN_HEIGHT / 2};

SDL_Rect outlineRect = {SCREEN_WIDTH / 6, SCREEN_HEIGHT / 6,
                        SCREEN_WIDTH * 2 / 3, SCREEN_HEIGHT * 2 / 3};

SDL_Rect screenRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

// viewports
SDL_Rect topLeftViewport = {0, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};

SDL_Rect topRightViewport = {SCREEN_WIDTH / 2, 0, SCREEN_WIDTH / 2,
                             SCREEN_WIDTH / 2};

SDL_Rect bottomViewport = {
    0,
    SCREEN_HEIGHT / 2,
    SCREEN_WIDTH,
    SCREEN_HEIGHT / 2,
};

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
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL) {
    printf("Could not create renderer :%s\n", SDL_GetError());
    return false;
  }

  // adjust renderer color used for various operations (?)
  SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);

  // start up sdl image loader
  int imageFlags = IMG_INIT_PNG; // this bitmask should result in a 1
  int initResult = IMG_Init(imageFlags) & imageFlags;
  if (!initResult) {
    printf("Could not load SDL image: %s\n", SDL_GetError());
    return false;
  }

  // get surface
  screenSurface = SDL_GetWindowSurface(window);

  return true;
}

SDL_Surface *LoadSurface(const char *path) {
  // as opposed to SDL_LoadBMP, image loads any format
  SDL_Surface *loadedSurface = IMG_Load(path);
  if (loadedSurface == NULL) {
    printf("Could not load surface: %s\n", SDL_GetError());
    return NULL;
  }

  // convert bmp to format of screen, so that this isn't done every blit
  // greatly optimizes rendering!
  SDL_Surface *optimizedSurface =
      SDL_ConvertSurface(loadedSurface, screenSurface->format, 0);
  if (optimizedSurface == NULL) {
    printf("Could not optimize surface: %s\n", SDL_GetError());
  }

  // get rid of original surface
  SDL_FreeSurface(loadedSurface);

  return optimizedSurface;
}

SDL_Texture *LoadTexture(const char *path) {

  // load raw surface image
  SDL_Surface *tSurface = IMG_Load(path);
  if (tSurface == NULL) {
    printf("Unable to load image: %s\n", SDL_GetError());
    return NULL;
  }

  // make texture from it
  SDL_Texture *nTexture = SDL_CreateTextureFromSurface(renderer, tSurface);

  // free original surface
  free(tSurface);

  return nTexture;
}

bool LoadMedia() {
  // laod all images
  // if any load fails, ret false
  images[KEY_PRESS_SURFACE_UP] = LoadTexture("../assets/states/test0.png");
  images[KEY_PRESS_SURFACE_DOWN] = LoadTexture("../assets/states/test1.png");
  images[KEY_PRESS_SURFACE_LEFT] = LoadTexture("../assets/states/test2.png");
  images[KEY_PRESS_SURFACE_RIGHT] = LoadTexture("../assets/states/test3.png");
  images[KEY_PRESS_SURFACE_DEFAULT] = LoadTexture("../assets/states/test4.png");

  for (int i = 0; i < KEY_PRESS_SURFACE_TOTAL; ++i) {
    if (images[i] == NULL) {
      return false;
    }
  }

  // set default image
  image = images[KEY_PRESS_SURFACE_DEFAULT];

  return true;
}

void Close() {
  // free asset mem
  SDL_DestroyTexture(image);
  image = NULL;

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
    return false;

  if (!LoadMedia())
    return false;

  // window loop
  SDL_Event e;
  bool quit = false;
  while (!quit) {
    // poll returns 0 when no events, only run loop if events in it
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        quit = true;
      }

      // set image depending on keydown
      else if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE:
          quit = true;
          break;
        case SDLK_UP:
          image = images[KEY_PRESS_SURFACE_UP];
          break;
        case SDLK_DOWN:
          image = images[KEY_PRESS_SURFACE_DOWN];
          break;
        case SDLK_LEFT:
          image = images[KEY_PRESS_SURFACE_LEFT];
          break;
        case SDLK_RIGHT:
          image = images[KEY_PRESS_SURFACE_RIGHT];
          break;
        default:
          image = images[KEY_PRESS_SURFACE_DEFAULT];
          break;
        }
      }
    }

    // clear screen
    SDL_RenderClear(renderer);

    // fill screen
    // why doesnt the call from l77 take effect here?
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderFillRect(renderer, &screenRect);

    // render texture
    // SDL_RenderCopy(renderer, image, NULL, NULL);

    // render viewports
    // call before rendering to draw relative to this viewport
    // as if it were the whole screen
    // SDL_RenderSetViewport(renderer, &topLeftViewport); 
    // SDL_RenderSetViewport(renderer, &topRightViewport); 
    // SDL_RenderSetViewport(renderer, &bottomViewport); 

    // render geometry
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0x00, 0xFF);
    SDL_RenderFillRect(renderer, &fillRect);

    SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 0xFF);
    SDL_RenderDrawRect(renderer, &outlineRect);

    SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
    SDL_RenderDrawLine(renderer, 0, SCREEN_HEIGHT / 2, SCREEN_WIDTH,
                       SCREEN_HEIGHT / 2);

    for (int i = 0; i < SCREEN_HEIGHT; i += 10) {
      SDL_RenderDrawPoint(renderer, SCREEN_WIDTH / 2, i);
    }
    
    // update screen
    SDL_RenderPresent(renderer);
  }

  return 1;
}
