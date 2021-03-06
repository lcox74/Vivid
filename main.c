#include "vivid.h"

int main (int argc, char* argv[]) {

    (void) argc;
    (void) argv;

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
        
        // Profiling draw functions
        VIVID_TIMER("Draw",
            // Draw to window
            vivid_draw_pixel(&_con, VIVID_POINT(100, 100), VIVID_RED);
            vivid_draw_rect(&_con, VIVID_RECT0(10, 10, 30, 30), VIVID_GREEN);
            vivid_draw_line(&_con, VIVID_POINT(350, 80), VIVID_POINT(200, 400), 
                VIVID_BLUE);
        );

        // Render buffer to screen
        vivid_render(&_con);
    }

    vivid_clean(&_con);

    return 0;
}