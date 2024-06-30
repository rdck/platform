/*******************************************************************************
 * display.h - simple drawing interface
 ******************************************************************************/

#pragma once

#include "linear_algebra.h"

#define COLOR_WHITE     0xFFFFFFFF
#define COLOR_BLACK     0xFF000000
#define COLOR_RED       0xFF0000FF
#define COLOR_GREEN     0xFF00FF00
#define COLOR_BLUE      0xFFFF0000

typedef U32 TextureID;

typedef struct {
  V2F ta;
  V2F tb;
  U32 color;
  V2F root;
  V2F size;
} Sprite;

// The option of supplying a lower render resolution is provided for the case
// of pixel art programs.
Void display_init(V2S window, V2S render);

U32 display_color(U8 r, U8 g, U8 b, U8 a);
U32 display_color_lerp(U32 a, U32 b, F32 t);

// The image is expected to be in 4-channel interleaved format.
TextureID display_load_image(const Byte* image, V2S dimensions);

Void display_begin_frame();
Void display_end_frame();

Void display_begin_draw(TextureID texture);
Void display_end_draw();

Void display_draw_sprite(V2F root, V2F size, U32 color, V2F t1, V2F t2);
Void display_draw_sprite_struct(Sprite sprite);
Void display_draw_texture(V2F root, V2F size, U32 color);
Void display_draw_texture_struct(Sprite sprite);
