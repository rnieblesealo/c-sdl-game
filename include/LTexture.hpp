// finish moving everything to this header!

#ifndef LTEXTURE_H
#define LTEXTURE_H

#include <SDL2/SDL.h>

class LTexture {
public:
  LTexture();
  ~LTexture();

  bool LoadFromFile(const char* path);
  void Free();
  
  void Render(int x, int y);

  int GetWidth();
  int GetHeight();

private:
  SDL_Texture* texture;

  int width;
  int height;
};

#endif
