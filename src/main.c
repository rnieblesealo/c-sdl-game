#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_video.h>
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

SDL_Surface *images[KEY_PRESS_SURFACE_TOTAL];
SDL_Surface *image = NULL;

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

  // i assume that this and results in an expression full of 1's
  int imageFlags = IMG_INIT_PNG;
  int initResult = IMG_Init(imageFlags) & imageFlags;
  if (!initResult) {
    printf("%016x", initResult);
    printf("Could not load SDL image: %s\n", SDL_GetError());
  }

  // get surface
  screenSurface = SDL_GetWindowSurface(window);

  return true;
}

SDL_Surface *LoadSurface(const char *path) {
  SDL_Surface *loadedSurface = SDL_LoadBMP(path);
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

bool LoadMedia() {
  // laod all images
  // if any load fails, ret false
  images[KEY_PRESS_SURFACE_UP] = LoadSurface("../assets/states/test0.bmp");
  images[KEY_PRESS_SURFACE_DOWN] = LoadSurface("../assets/states/test1.bmp");
  images[KEY_PRESS_SURFACE_LEFT] = LoadSurface("../assets/states/test2.bmp");
  images[KEY_PRESS_SURFACE_RIGHT] = LoadSurface("../assets/states/test3.bmp");
  images[KEY_PRESS_SURFACE_DEFAULT] = LoadSurface("../assets/states/test4.bmp");

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
  SDL_FreeSurface(image);
  image = NULL;

  // free window mem
  SDL_DestroyWindow(window);
  window = NULL;

  // terminate sdl subsystem
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
  while (quit == false) {
    // fill window with color
    // matchrgb gets the pixel format taken by the screen, the r, g, b
    // want, and maps it to a pixel value
    SDL_FillRect(screenSurface, NULL,
                 SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));

    // draw image, stretching to fill
    SDL_Rect stretch;

    stretch.x = 0;
    stretch.y = 0;
    stretch.w = SCREEN_WIDTH;
    stretch.h = SCREEN_HEIGHT;

    SDL_BlitScaled(image, NULL, screenSurface, &stretch);

    // update
    SDL_UpdateWindowSurface(window);

    // poll returns 0 when no events, only run loop if events in it
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        quit = true;
      }

      // set image depending on keydown
      else if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
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
  }

  return 1;
}
