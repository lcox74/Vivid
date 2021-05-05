# VIVID Graphics
> 5 May 2021

VIVID is my single header library to abstract handling and initialising SDL2 in
a project. This isn't the most efficient way to use SDL2 as VIVID allows the 
user to draw pixels to a frame buffer before rendering to the window.

I started getting annoyed when I had to keep spending the first `x` number of 
hours of a project just seting up a renderer to draw to, so I decided to create
this abstraction.

## Usage

### Windows

As a single header library all you have to do is include the `include/vivid.h` 
to your project. Unfortunately because this runs on SDL2 you will also need to
add the include directory and link the SDL2 libs to the project and add the 
`SDL2.dll` to the executable directory.

### Linux

This is currently un-tested as my current linux machine is terminal based. But
as long as you have SDL2 installed e.g.

```BASH
sudo apt-get install libsdl2-dev
```

Then all you need to do is include the `include/vivid.h` and add `-lSDL2` to
your build system (Makefile). Then it should just work.

## Example

The following is a minimal example that creates a `800x600` window and draws a
red pixel at the point `(100, 100)`.

```C
#include "vivid.h"

int main (int argc, char* argv[]) {

    // Create VIVID context
    vivid_context _con = vivid_create("Vivid Example", 800, 600, 0);
    
    Uint8 running = 1;
    SDL_Event event;

    // Render Loop
    while (running) {
        // Handle Events 
        while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_QUIT) {
                running = 0; 
                break;
            }
		}

        // Clear the window
        vivid_clear(&_con);

        // Draw red to point (100, 100)
        vivid_draw_pixel(&_con, VIVID_POINT(100, 100), VIVID_RED);

        // Render buffer to screen
        vivid_render(&_con);
    }

    vivid_clean(&_con);

    return 0;
}
```

## TODO

- Colour Abstraction
- Event Handling
- Text Rendering