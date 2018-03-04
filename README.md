## ogl_template

Template for OpenGL projects that render triangles. Uses a buffer of position, color, normal and UV, which is squished to 24 bytes per vertex by using various normalized ints.

To compile this, you will need a `deps` folder. I'm doing something a little unusual here, so that I can quickly create prototype projects. My deps folder has the subdirectories `chipmunk`, `glad`, `portaudio`, `sdl2` and `stb`. Under `stb` I dumped the whole [stb repo](https://github.com/nothings/stb). Under `glad` I dumped the `c` branch of the [glad repo](https://github.com/Dav1dde/glad/tree/c). The `chipmunk`, `sdl2` and `portaudio` projects are structured with `include` and `lib` folders. Under the `lib` folders are subdirectories `macos`, `linux` and `win32` which should contain the static libs to link for their respective platforms.

It's a pain, but it ended up being better for me than trying to manage git sub-repos.

[Here's a link](https://www.dropbox.com/s/7xykn54aiq8nvs6/deps.zip?dl=0) to a deps folder that I use on macOS.

The project uses cmake, so all you have to do once you set up the `deps` folder is:

```
mkdir build
cd build
cmake ..
make
```
