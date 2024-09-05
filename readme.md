# Problems

- [ ] `renderDest` is used as collider, but it starts off as 0, 0, 0, 0
- [x] Player collision check clipping
- [x] Get everything ready for next tutorial
- [ ] Fix camera/scaling, *see sect. in notes*

# Notes

### Camera/Scaling
Because we scale everything at render-time but the original thing is actually a very small image, a camera that moves a clip rect around isn't viable, even if the dest. rect stretches it to the desired size.

We'd have to make the camera work with these unscaled coordinates, which limits us to pixel-perfect camera movement. 

The easiest solution to this problem is to just load the textures with the correct size already.

Specifically, the background texture, because we want to move that src. rect freely around it, and it would help us keep the camera in world-relative coordinates.

### Pixel-Perfect Collision
I'm going to skip this; it's unnecessary for the game I'm making, and it's trivial to figure out how to implement it. See the page for future reference.

### Adjusted Frame Stepping
This is how we framestep our game according to our adjusted FPS

1. Take last update time
2. Take current time
3. Use to compute $dt$ 
4. If $dt \lt \dfrac{1}{fps.}$, repeat from step 2, otherwise continue
5. (Do update logic) 
5. Set last update time to be the current time
6. Repeat!

### String Literals
These are any strings created in function calls, etc.

- Live in read-only memory
- Live throughout the entire program's lifetime
- When literal is made, sequence is used to init **static storage** arr. to contain it
- *If we try to modify this array, we get undefined behavior*
    - That's why we use `const char *`!

### Byte Alignment
32-bit CPUs, for example, read in 4-byte words, cyclically

If we have 2 contiguous bytes, we'd need 2 cycles to read them

the first 4-byte read occludes byte 2 

But if we pad byte 2 by 3 bytes ahead (so that it isn't occluded by byte 1 in a read), we can read *both* in *1 cycle*

This is what alignment is (in my understanding); padding to make addressing more efficient
