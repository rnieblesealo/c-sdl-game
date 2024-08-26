#include <SDL.h>
#include <SDL_error.h>
#include <SDL_events.h>
#include <SDL_surface.h>
#include <SDL_video.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

const int SCREEN_WIDTH = 234;
const int SCREEN_HEIGHT = 234;

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

  // get surface
  screenSurface = SDL_GetWindowSurface(window);

  return true;
}

SDL_Surface *LoadImage(const char *path) {
  // loads a single bitmap image
  image = SDL_LoadBMP(path);
  if (!image) {
    printf("Unable to load image: %s\n", SDL_GetError());
    return NULL;
  }

  return image;
}

bool LoadMedia() {
  // laod all images
  // if any load fails, ret false
  images[KEY_PRESS_SURFACE_UP] = LoadImage("../assets/states/test0.bmp");
  images[KEY_PRESS_SURFACE_DOWN] = LoadImage("../assets/states/test1.bmp");
  images[KEY_PRESS_SURFACE_LEFT] = LoadImage("../assets/states/test2.bmp");
  images[KEY_PRESS_SURFACE_RIGHT] = LoadImage("../assets/states/test3.bmp");
  images[KEY_PRESS_SURFACE_DEFAULT] = LoadImage("../assets/states/test4.bmp");

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

    // draw image
    SDL_BlitSurface(image, NULL, screenSurface, NULL);

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
          printf("uppy!\n");
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
