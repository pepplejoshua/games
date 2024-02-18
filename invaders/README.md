# invaders
Learnt from [here](https://nicktasios.nl/posts/space-invaders-from-scratch-part-1.html).

## required installations
I installed `glfw` and `glew` with `brew install glfw glew` but that does not seem to work at all. So we will go to the official website to get [binaries](https://www.glfw.org/download.html).

I have already included the glfw3.h and glfw3native.h in the lib folder. All you need to do is figure out which platform you use and go into the right folder. On my M2 Mac Mini which run an arm64 chip, I will go into the lib-arm64 folder and grab the libglfw3.a file and put it in the lib folder. The `make` command should work now. 

`Note`: this has only been tested on macOS. I expect there to be variations (changing Makefile to reference libglfw3.dylib for Windows, and grabbing that file from the right architecture folder).