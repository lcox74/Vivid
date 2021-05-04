#include <stdio.h>
#include <windows.h>

#include "SDL2/SDL.h"

#define VIVID_OK        0
#define VIVID_FAIL      1

#define VIVID_PANIC(mess, code) {                                             \
    printf("VIVID PANIC (%d)\n\t%s", code, mess);                             \
    exit(code);                                                               \
}

typedef struct _vivid_colour {
    union {
        Uint32 hex;
        struct {
            Uint8 r, g, b, a;
        };
    };
} vivid_colour; // RGBA

typedef struct _vivid_rect {
    Uint16 x, y, w, h;
} vivid_rect; // Rect ([x, y], [x + w, y + h])

#define VIVID_POINT(x0,y0)                                                    \
    ((vivid_rect) { .x = (x0), .y = (y0), .w = 0, .h = 0 })
#define VIVID_RECT0(x0,y0,x1,y1)                                               \
    ((vivid_rect) { .x = (x0), .y = (y0), .w = ((x1)-(x0)), .h = ((y1)-(y0)) })
#define VIVID_RECT1(x0,y0,w0,h0)                                               \
    ((vivid_rect) { .x = (x0), .y = (y0), .w = (w0), .h = (h0) })

typedef struct _vivid_context{
    const char*     window_title;
    vivid_rect      window_size;
    vivid_colour    window_clear_colour;
    Uint32          window_flags;

    vivid_colour*   window_buffer;

    SDL_Window*     _window;
    SDL_Renderer*   _renderer;
    SDL_Texture*    _texture;
} vivid_context;

#define VIVID_GREY      ((vivid_colour) { .hex = 0xFF222222 })
#define VIVID_RED       ((vivid_colour) { .hex = 0xFF0000FF })
#define VIVID_GREEN     ((vivid_colour) { .hex = 0xFF00FF00 })
#define VIVID_BLUE      ((vivid_colour) { .hex = 0xFFFF0000 })

Uint8 _vivid_window_create(vivid_context* context);
Uint8 _vivid_renderer_create(vivid_context* context);
Uint8 _vivid_texture_create(vivid_context* context);

vivid_context vivid_create(const char*, Uint16, Uint16, Uint32);
Uint8 vivid_intialise(vivid_context* context);
Uint8 vivid_clean(vivid_context* context);

Uint8 vivid_clear(vivid_context* context);
Uint8 vivid_render(vivid_context* context);

Uint8 vivid_draw_pixel(vivid_context* context, vivid_rect, vivid_colour);
Uint8 vivid_draw_rect(vivid_context* context, vivid_rect, vivid_colour);
Uint8 vivid_draw_line(vivid_context* context, vivid_rect, vivid_rect, 
    vivid_colour);

Uint8 vivid_blur(vivid_context* context, Uint8);

vivid_context vivid_create(const char* title, Uint16 width, Uint16 height, 
        Uint32 flags) {
    vivid_context _context = (vivid_context) { .window_title = title };
    _context.window_size = (vivid_rect) { 
        .x = 0, 
        .y = 0, 
        .w = width, 
        .h = height 
    };
    _context.window_clear_colour = VIVID_GREY;
    _context.window_flags = flags;

    return _context;
}

Uint8 vivid_intialise(vivid_context* context) {
    // Initialisation
    if (SDL_Init(SDL_INIT_VIDEO) < 0) 
        VIVID_PANIC("SDL2 couldn't initialise", VIVID_FAIL);

    _vivid_window_create(context);
    _vivid_renderer_create(context);
    _vivid_texture_create(context);
    
    size_t bs = sizeof(vivid_colour) * context->window_size.w * 
        context->window_size.h;
    context->window_buffer = (vivid_colour*) malloc(bs);

    memset(context->window_buffer, VIVID_GREY.hex, bs);

    return 0;
}

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

    return 0;
}

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

    return 0;
}

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

    return 0;
}

Uint8 vivid_clean(vivid_context* context) {
    SDL_DestroyTexture(context->_texture);
    SDL_DestroyRenderer(context->_renderer);
    SDL_DestroyWindow(context->_window);
    SDL_Quit();

    return 0;
}

Uint8 vivid_clear(vivid_context* context) {
    // Clear Window
    SDL_SetRenderDrawColor(
        context->_renderer, 
        context->window_clear_colour.r,
        context->window_clear_colour.g,
        context->window_clear_colour.b,
        context->window_clear_colour.a
    );
    SDL_RenderClear(context->_renderer);

    return 0;
}

Uint8 vivid_render(vivid_context* context) {
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
    SDL_RenderCopy(context->_renderer, context->_texture, NULL, NULL);

    // Render Frame
    SDL_RenderPresent(context->_renderer);

    return 0;
}

Uint8 vivid_draw_pixel(vivid_context* context, vivid_rect p, vivid_colour c) {

    if (p.x > context->window_size.w || p.y > context->window_size.h)
        return 1;

    context->window_buffer[p.y * context->window_size.w + p.x] = c;

    return 0;
}

Uint8 vivid_draw_rect(vivid_context* context, vivid_rect p, vivid_colour c) {
    if (p.x > context->window_size.w || p.y > context->window_size.h ||
            p.x + p.w > context->window_size.w || 
            p.y + p.w > context->window_size.h)
        return 1;

    Uint16 index = 0;
    for (Uint16 y = p.y; y < p.y + p.h; y++) {
        for (Uint16 x = p.x; x < p.x + p.w; x++) {
            index = y * context->window_size.w + x;
            context->window_buffer[index] = c;
        }
    }

    return 0;
}

Uint8 vivid_draw_line(vivid_context* context, vivid_rect p0, vivid_rect p1, 
        vivid_colour c) {

    if (p0.x > context->window_size.w || p0.y > context->window_size.h ||
            p1.x > context->window_size.w || p1.y > context->window_size.h)
        return 1;

    int dx, dy, sx, sy, err, e2;

    dx = abs(p1.x - p0.x);
    sx = p0.x < p1.x ? 1 : -1;
    dy = -abs(p1.y - p0.y);
    sy = p0.y < p1.y ? 1 : -1;
    err = dx + dy;

    while (1) {
        vivid_draw_pixel(context, p0, c);
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

    return 0;    
}

Uint8 vivid_blur(vivid_context* context, Uint8 amount) {
    size_t bs = sizeof(vivid_colour) * context->window_size.w * 
        context->window_size.h;
    vivid_colour* buffer = (vivid_colour*) malloc(bs);
    memset(buffer, VIVID_GREY.hex, bs);

    int sum = 0, index = 0;
    for (int x = 0; x < context->window_size.w; x++) {
        for (int y = 0; y < context->window_size.h; y++) {
            vivid_colour avg = context->window_buffer[y * 
                context->window_size.w + x];
            sum = 0;

            for (int nx = -(amount / 2); nx <= (amount / 2); nx++) {
                for (int ny = -(amount / 2); ny <= (amount / 2); ny++) {
                    if (x + nx >= 0 && y + ny >= 0 &&
                            x + nx < context->window_size.w && 
                            y + ny < context->window_size.h) {
                        
                        index = (y + ny) * context->window_size.w + (x + nx);
                        avg.r += context->window_buffer[index].r;
                        avg.g += context->window_buffer[index].g;
                        avg.b += context->window_buffer[index].b;
                        sum++;
                    }
                }
            }
            avg.r /= sum;
            avg.g /= sum;
            avg.b /= sum;
            buffer[y * context->window_size.w + x] = avg;
        }
    }

    memcpy(context->window_buffer, buffer, bs);
    return 1;
}