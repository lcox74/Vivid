#include <stdio.h>
#include <sys/timeb.h>

#if defined(_WIN32) || defined(WIN32) 
    #include <windows.h>
#endif

#include "SDL2/SDL.h"

#define VIVID_OK        0
#define VIVID_FAIL      1

// #define VIVID_TRACE(fmt, args...) printf(fmt, ##args)

/* Panics the program and print the reason for panic before exiting */
#define VIVID_PANIC(mess, code) {                                             \
    printf("VIVID PANIC (%d)\n\t%s", code, mess);                             \
    exit(code);                                                               \
}

/* Used to time performance of a chunk of code */
#define VIVID_TIMER(proc, code) {                                             \
    struct timeb s, e;                                                        \
    Uint64 diff;                                                              \
    ftime(&s);                                                                \
    {code}                                                                    \
    ftime(&e);                                                                \
    diff = (int) (1000.0 * (e.time - s.time) + (e.millitm - s.millitm));      \
    printf("VIVID TIMER (%s) [%lums]\n", proc, diff);                         \
}

#define VIVID_ASSERT_CONTEXT(context) {                                       \
    if (!((context)->_window) ||                                              \
        !((context)->_renderer) ||                                            \
        !((context)->_texture)) {                                             \
            printf("VIVID Context not Initialise");                           \
            return VIVID_FAIL;                                                \
    }                                                                         \
}
typedef struct _vivid_colour {
    union {
        Uint32 hex;
        struct {
            Uint8 r, g, b, a;
        };
    };
} vivid_colour; // ABGR8888 Format

typedef struct _vivid_rect {
    Uint16 x, y, w, h;
} vivid_rect; // Rect ([x, y], [x + w, y + h])

#define VIVID_POINT(x0,y0)                                                    \
    ((vivid_rect) { .x = (x0), .y = (y0), .w = 0, .h = 0 })
#define VIVID_RECT0(x0,y0,x1,y1)                                              \
    ((vivid_rect) { .x = (x0), .y = (y0), .w = ((x1)-(x0)), .h = ((y1)-(y0)) })
#define VIVID_RECT1(x0,y0,w0,h0)                                              \
    ((vivid_rect) { .x = (x0), .y = (y0), .w = (w0), .h = (h0) })

typedef struct _vivid_context{
    const char*     window_title;
    vivid_rect      window_size;
    vivid_colour    window_clear_colour;
    Uint32          window_flags;

    vivid_colour*   window_buffer;

    /* SDL components */
    SDL_Window*     _window;
    SDL_Renderer*   _renderer;
    SDL_Texture*    _texture;
} vivid_context;

/* 8-Bit Colour Pallet from https://en.wikipedia.org/wiki/ANSI_escape_code*/
#define VIVID_BLACK            ((vivid_colour) { .hex = 0xFF000000 })
#define VIVID_RED              ((vivid_colour) { .hex = 0xFF3131CD })
#define VIVID_GREEN            ((vivid_colour) { .hex = 0xFF79BC0D })
#define VIVID_YELLOW           ((vivid_colour) { .hex = 0xFF10E5E5 })
#define VIVID_BLUE             ((vivid_colour) { .hex = 0xFFC87224 })
#define VIVID_MAGENTA          ((vivid_colour) { .hex = 0xFFBC3FBC })
#define VIVID_CYAN             ((vivid_colour) { .hex = 0xFFCDA811 })
#define VIVID_WHITE            ((vivid_colour) { .hex = 0xFFE5E5E5 })
#define VIVID_BRIGHT_BLACK     ((vivid_colour) { .hex = 0xFF666666 })
#define VIVID_BRIGHT_RED       ((vivid_colour) { .hex = 0xFF4C4CF1 })
#define VIVID_BRIGHT_GREEN     ((vivid_colour) { .hex = 0xFF8BD123 })
#define VIVID_BRIGHT_YELLOW    ((vivid_colour) { .hex = 0xFF43F5F5 })
#define VIVID_BRIGHT_BLUE      ((vivid_colour) { .hex = 0xFFEA8E3B })
#define VIVID_BRIGHT_MAGENTA   ((vivid_colour) { .hex = 0xFFD670D6 })
#define VIVID_BRIGHT_CYAN      ((vivid_colour) { .hex = 0xFFDBB829 })
#define VIVID_BRIGHT_WHITE     ((vivid_colour) { .hex = 0xFFE5E5E5 })

/* Creation/Initialisation and cleaning of the VIVID system*/
vivid_context vivid_create(const char*, Uint16, Uint16, Uint32);
Uint8 vivid_clean(vivid_context*);

/* SDL initialisers  */
Uint8 _vivid_intialise(vivid_context*);
Uint8 _vivid_window_create(vivid_context*);
Uint8 _vivid_renderer_create(vivid_context*);
Uint8 _vivid_texture_create(vivid_context*);

/* Base functions to clear or swap window buffers */
Uint8 vivid_clear(vivid_context*);
Uint8 vivid_render(vivid_context*);
Uint8 vivid_set_clear_colour(vivid_context*, vivid_colour);

/* Base draw functions */
Uint8 vivid_draw_pixel(vivid_context*, vivid_rect, vivid_colour);
Uint8 vivid_draw_rect(vivid_context*, vivid_rect, vivid_colour);
Uint8 vivid_draw_line(vivid_context*, vivid_rect, vivid_rect, vivid_colour);
Uint8 vivid_draw_sprite(vivid_context*, vivid_rect, vivid_colour*);

/*
 * Create the base context structure that stores the required data to abstract
 * the initialisation and handling of SDL.
 */
vivid_context vivid_create(const char* title, Uint16 width, Uint16 height, 
        Uint32 flags) {
    
    // Create the main context data structure based on parameters
    vivid_context _context = (vivid_context) { .window_title = title };
    _context.window_size = (vivid_rect) { 
        .x = 0, 
        .y = 0, 
        .w = width, 
        .h = height 
    };
    _context.window_clear_colour = VIVID_BLACK;
    _context.window_flags = flags;

    // Create a colour buffer to be able to edit between frames
    size_t bs = sizeof(vivid_colour) * width * height;
    _context.window_buffer = (vivid_colour*) malloc(bs);

    // Set the base colour (clear colour)
    memset(_context.window_buffer, VIVID_BLACK.hex, bs);

    // Initialise the SDL components
    _vivid_intialise(&_context);

    return _context;
}

/*
 * Initialise SDL and set up the window which will allow for custom buffer 
 * rendering. If the base initialisation fails then the program will exit
 * after leaving a reason why it failed.
 */
Uint8 _vivid_intialise(vivid_context* context) {
    // Initialisation
    if (SDL_Init(SDL_INIT_VIDEO) < 0) 
        VIVID_PANIC("SDL2 couldn't initialise", VIVID_FAIL);

    // Base SDL creation
    _vivid_window_create(context);
    _vivid_renderer_create(context);
    _vivid_texture_create(context);
    
    return VIVID_OK;
}

/*
 * Use SDL to create a window with the properties of the supplied context.
 * If the window creation fails then the program will panic after printing
 * the SDL error.
 */
Uint8 _vivid_window_create(vivid_context* context) {
    // Window Creation
    context->_window = SDL_CreateWindow(
        context->window_title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        context->window_size.w,
        context->window_size.h,
        context->window_flags
    );

    // Check successful window creation
    if (!context->_window) 
        VIVID_PANIC(SDL_GetError(), VIVID_FAIL);

    return VIVID_OK;
}

/*
 * Use SDL to create a renderer that will utilise software based renderering.
 * On failure the program will panic (exit) after printing the SDL error.
 */
Uint8 _vivid_renderer_create(vivid_context* context) {
    // Renderer Creation
    context->_renderer = SDL_CreateRenderer(
        context->_window,
        -1,
        SDL_RENDERER_SOFTWARE
    );

    // Check successful renderer creation
    if (!context->_renderer)
        VIVID_PANIC(SDL_GetError(), VIVID_FAIL);

    return VIVID_OK;
}

/*
 * Use SDL to create a texture that will be used to pull and push a buffer of
 * colour data in the format of ABGR8888. On faliure the program will panic 
 * after printing the SDL error.
 */
Uint8 _vivid_texture_create(vivid_context* context) {
    // Texture Creation
    context->_texture = SDL_CreateTexture(
        context->_renderer,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        context->window_size.w,
        context->window_size.h
    );

    // Check successful texture creation
    if (!context->_texture)
        VIVID_PANIC(SDL_GetError(), VIVID_FAIL);

    return VIVID_OK;
}

/*
 * Clean the vivid context by destroying the SDL components.
 */
Uint8 vivid_clean(vivid_context* context) {
    VIVID_ASSERT_CONTEXT(context);

    SDL_DestroyTexture(context->_texture);
    SDL_DestroyRenderer(context->_renderer);
    SDL_DestroyWindow(context->_window);
    SDL_Quit();

    return VIVID_OK;
}

/*
 * Clear the screen buffer setting the screen to the clear colour.
 */
Uint8 vivid_clear(vivid_context* context) {
    VIVID_ASSERT_CONTEXT(context);

    // Clear Window
    SDL_SetRenderDrawColor(
        context->_renderer, 
        context->window_clear_colour.r,
        context->window_clear_colour.g,
        context->window_clear_colour.b,
        context->window_clear_colour.a
    );
    SDL_RenderClear(context->_renderer);

    return VIVID_OK;
}

/*
 * Copy the context image buffer to the SDL screen buffer and present the 
 * new frame to the window.
 */
Uint8 vivid_render(vivid_context* context) {
    VIVID_ASSERT_CONTEXT(context);

    Uint32* pixels;
    Uint32  pitch;

    // Construct Frame
    SDL_LockTexture(
        context->_texture,
        NULL,
        (void**)&pixels,
        (int*)&pitch
    );

    // Set pixel colour
    memcpy(pixels, context->window_buffer, sizeof(vivid_colour) * 
        context->window_size.w * context->window_size.h);

    SDL_UnlockTexture(context->_texture);

    // Render Frame
    SDL_RenderCopy(context->_renderer, context->_texture, NULL, NULL);
    SDL_RenderPresent(context->_renderer);

    return VIVID_OK;
}

Uint8 vivid_set_clear_colour(vivid_context* context, vivid_colour c) {
    VIVID_ASSERT_CONTEXT(context);

    context->window_clear_colour = c;
    return VIVID_OK;
}

/*
 * Draw a single pixel at point `p` (x,y) with the colour of `c` to the window
 * buffer. If the colour `c` has an alpha less than 255, then it will alpha
 * blend the final colour accordingly.
 */
Uint8 vivid_draw_pixel(vivid_context* context, vivid_rect p, vivid_colour c) {
    VIVID_ASSERT_CONTEXT(context);

    // Don't render anything out of the window buffer
    if (p.x >= context->window_size.w || p.y >= context->window_size.h)
        return VIVID_FAIL;

    const size_t index = p.y * context->window_size.w + p.x;
    const vivid_colour buf = context->window_buffer[index];

    vivid_colour final_colour;
    const float a = c.a / 255.0f;
    const float ca = 1.0f - a;

    // Alpha blend new colour onto previous colour
    final_colour.r = (a * (float)c.r) + (ca * (float)buf.r);
    final_colour.g = (a * (float)c.g) + (ca * (float)buf.g);
    final_colour.b = (a * (float)c.b) + (ca * (float)buf.b);
    final_colour.a = 255;

    // Place main pixel
    context->window_buffer[index] = final_colour;

    return VIVID_OK;
}

/*
 * Render a filled rectangle at point and size `p` (x, y, width, height) with
 * colour of `c`. If part of the rectangle is outside the window buffer then it
 * will draw all the pixels instead. 
 */
Uint8 vivid_draw_rect(vivid_context* context, vivid_rect p, vivid_colour c) {
    VIVID_ASSERT_CONTEXT(context);

    for (Uint16 y = p.y; y < p.y + p.h; y++) {
        for (Uint16 x = p.x; x < p.x + p.w; x++) {
            vivid_draw_pixel(context, VIVID_POINT(x, y), c);
        }
    }

    return VIVID_OK;
}

/*
 * Render a line from point `p0` to `p1` (x, y) with colour `c`. This uses the 
 * Bresenham's line algorithm. If part of the line is outside the window buffer
 * then it will render all the points that are in the window buffer.
 */
Uint8 vivid_draw_line(vivid_context* context, vivid_rect p0, vivid_rect p1, 
        vivid_colour c) {

    VIVID_ASSERT_CONTEXT(context);

    int dx, dy, sx, sy, err, e2;

    dx = abs(p1.x - p0.x);
    sx = p0.x < p1.x ? 1 : -1;
    dy = -abs(p1.y - p0.y);
    sy = p0.y < p1.y ? 1 : -1;
    err = dx + dy;

    while (1) {
        vivid_draw_pixel(context, p0, c); // Change to weighted pixel later
        if (p0.x == p1.x && p0.y == p1.y) break;

        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            p0.x += sx;
        }
        if (e2 <= dx) {
            err += dx;
            p0.y += sy;
        }
    }

    return VIVID_OK;    
}

/*
 * Render a sprite (colour buffer) at point `p` of size `p`. Vivid won't render
 * any points outside the window buffer, but will partially render.
 */
Uint8 vivid_draw_sprite(vivid_context* context, vivid_rect p, 
        vivid_colour* buf) {

    VIVID_ASSERT_CONTEXT(context);

    for (int y = 0; y < p.h; y++) {
        for (int x = 0; x < p.w; x++) {
            vivid_draw_pixel(context, VIVID_POINT(p.x + x, p.y + y), 
                buf[y * p.w + x]);
        }
    }

    return VIVID_OK;
}