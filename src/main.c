#include <SDL.h>
#include <SDL_error.h>
#include <SDL_events.h>
#include <SDL_surface.h>
#include <SDL_video.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

SDL_Window *window = NULL; // render target
SDL_Surface *screenSurface =
    NULL; // surface (2d image) contained by render target

// assets
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

  // get surface
  screenSurface = SDL_GetWindowSurface(window);

  return true;
}

bool LoadAssets() {
  image = SDL_LoadBMP("../assets/image.bmp");
  if (image == NULL) {
    printf("Unable to load image: %s", SDL_GetError());
    return false;
  }
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

  if (!LoadAssets())
    return false;

  // fill window with color
  // matchrgb gets the pixel format taken by the screen, the r, g, b values we
  // want, and maps it to a pixel value
  SDL_FillRect(screenSurface, NULL,
               SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));

  // draw image
  SDL_BlitSurface(image, NULL, screenSurface, NULL);

  // update
  SDL_UpdateWindowSurface(window);

  // window loop
  SDL_Event e;
  bool quit = false;
  while (quit == false) {
    // poll returns 0 when no events, only run loop if events in it
    while (SDL_PollEvent(&e) != 0) {
      if (e.type == SDL_QUIT) {
        quit = true;
      }
    }
  }

  return 1;
}
