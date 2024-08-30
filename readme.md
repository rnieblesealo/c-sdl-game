# Notes

### Adjusted Frame Stepping
This is how we framestep our game according to our adjusted FPS

1. Take last update time
2. Take current time
3. Use to compute $dt$ 
4. If $dt \lt \dfrac{1}{fps.}$, repeat from step 2, otherwise continue
5. (Do update logic) 
5. Set last update time to be the current time
6. Repeat!
