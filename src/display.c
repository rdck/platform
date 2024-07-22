#include <stdlib.h>
#include "display.h"
#include "log.h"
#include "glad/gl.h"
#include "shader/sprite.vert.h"
#include "shader/sprite.frag.h"
#include "shader/resample.vert.h"
#include "shader/resample.frag.h"

#ifndef DISPLAY_SPRITES
#define DISPLAY_SPRITES 0x400
#endif

#ifndef DISPLAY_TEXTURE_FILTER
#define DISPLAY_TEXTURE_FILTER GL_LINEAR
#endif

#ifndef DISPLAY_FB_FILTER
#define DISPLAY_FB_FILTER GL_NEAREST
#endif

#define DISPLAY_SPRITE_VERTICES 6 // two triangles
#define DISPLAY_VERTICES (DISPLAY_SPRITES * DISPLAY_SPRITE_VERTICES)
#define DISPLAY_LOG_LENGTH 0x800
#define DISPLAY_RENDER_STRIDE 5
#define DISPLAY_POSTPROCESS_STRIDE 4

typedef struct {

  V2S render_resolution;
  V2S window_resolution;

  GLuint render_program;
  GLuint render_vbo;
  GLuint render_vao;

  GLuint resample_program;
  GLuint resample_vbo;
  GLuint resample_vao;

  GLuint fbo;
  GLuint fb_color;

  GLint projection;

} DisplayContext;

typedef struct Vertex {
  F32 x; F32 y; // position
  F32 u; F32 v; // texture coordinates
  U32 color;
} Vertex;

typedef struct ShaderSource {
  GLchar* data;
  GLint length;
} ShaderSource;

static Sprite display_sprite_buffer[DISPLAY_SPRITES] = {0};
static Vertex display_vertex_buffer[DISPLAY_VERTICES] = {0};
static S32 display_sprite_index = 0;

static DisplayContext ctx = {0};

static F32 quad_vertices[] = {  
  -1.0f, +1.0f, +0.0f, +1.0f,
  -1.0f, -1.0f, +0.0f, +0.0f,
  +1.0f, -1.0f, +1.0f, +0.0f,

  -1.0f, +1.0f, +0.0f, +1.0f,
  +1.0f, -1.0f, +1.0f, +0.0f,
  +1.0f, +1.0f, +1.0f, +1.0f,
};

static Void GLAPIENTRY gl_message_callback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const Void* user_param
    )
{
  UNUSED_PARAMETER(source);
  UNUSED_PARAMETER(type);
  UNUSED_PARAMETER(id);
  UNUSED_PARAMETER(severity);
  UNUSED_PARAMETER(length);
  UNUSED_PARAMETER(user_param);
  platform_log_warn(message);
}

TextureID display_load_image(const Byte* image, V2S dimensions)
{
  ASSERT(image);
  GLuint id = 0;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, DISPLAY_TEXTURE_FILTER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, DISPLAY_TEXTURE_FILTER);
#ifdef DISPLAY_SUPPORT_MONOCHROME
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
#endif
  glTexImage2D(
      GL_TEXTURE_2D,
      0,                  // level
      GL_RGBA,            // internal format
      dimensions.x,       // width
      dimensions.y,       // height
      0,                  // border (must be 0)
      GL_RGBA,            // format
      GL_UNSIGNED_BYTE,   // type of pixel data
      image
      );
  return id;
}

static GLuint compile_shader(GLenum type, ShaderSource source)
{
  const GLuint shader_id = glCreateShader(type);
  ASSERT(shader_id);

  GLchar* const sources[1] = { source.data };
  glShaderSource(shader_id, 1, sources, &source.length);
  glCompileShader(shader_id);

  GLint success = 0;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
  if (success == 0) {
    Char log_buffer[DISPLAY_LOG_LENGTH];
    GLsizei log_length = 0;
    glGetShaderInfoLog(shader_id, DISPLAY_LOG_LENGTH, &log_length, log_buffer);
    platform_log_error(log_buffer);
  }

  return shader_id;
}

static GLuint link_program(GLuint vertex, GLuint fragment)
{
  const GLuint program = glCreateProgram();
  ASSERT(program);

  glAttachShader(program, vertex);
  glAttachShader(program, fragment);
  glLinkProgram(program);

  GLint success = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (success == 0) {
    Char log_buffer[DISPLAY_LOG_LENGTH];
    GLsizei log_length = 0;
    glGetProgramInfoLog(program, DISPLAY_LOG_LENGTH, &log_length, log_buffer);
    platform_log_error(log_buffer);
  }

  return program;
}

static GLuint compile_program(ShaderSource vert, ShaderSource frag)
{
  const GLuint vert_id = compile_shader(GL_VERTEX_SHADER, vert);
  const GLuint frag_id = compile_shader(GL_FRAGMENT_SHADER, frag);
  ASSERT(vert_id);
  ASSERT(frag_id);

  const GLuint program = link_program(vert_id, frag_id);
  ASSERT(program);

  return program;
}

// @rdk: disable GL debug output in release builds
Void display_init(V2S window, V2S render)
{
  ctx.window_resolution = window;
  ctx.render_resolution = render;

  glEnable(GL_DEBUG_OUTPUT);
  glDebugMessageCallback(gl_message_callback, 0);

  const ShaderSource vert_primary = { (GLchar*) shader_sprite_vertex      , shader_sprite_vertex_len      };
  const ShaderSource frag_primary = { (GLchar*) shader_sprite_fragment    , shader_sprite_fragment_len    };
  const ShaderSource vert_post    = { (GLchar*) shader_resample_vertex    , shader_resample_vertex_len    };
  const ShaderSource frag_post    = { (GLchar*) shader_resample_fragment  , shader_resample_fragment_len  };

  ctx.render_program = compile_program(vert_primary, frag_primary);
  ctx.resample_program = compile_program(vert_post, frag_post);

  ////////////////////////////////////////////////////////////////////////////////

  glUseProgram(ctx.resample_program);

  glGenVertexArrays(1, &ctx.resample_vao);
  glGenBuffers(1, &ctx.resample_vbo);
  glBindVertexArray(ctx.resample_vao);
  glBindBuffer(GL_ARRAY_BUFFER, ctx.resample_vbo);

  {
    const GLsizei stride = DISPLAY_POSTPROCESS_STRIDE * sizeof(F32);
    glVertexAttribPointer(
        0,                          // index
        2,                          // size
        GL_FLOAT,                   // type
        GL_FALSE,                   // normalized
        stride,                     // stride
        (Void*)0                    // offset
        );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1,                          // index
        2,                          // size
        GL_FLOAT,                   // type
        GL_FALSE,                   // normalized
        stride,                     // stride
        (Void*)(2 * sizeof(F32))    // offset
        );
    glEnableVertexAttribArray(1);
  }

  glBufferData(
      GL_ARRAY_BUFFER,        // target
      sizeof(quad_vertices),  // size in bytes
      quad_vertices,          // data
      GL_STATIC_DRAW          // usage
      );

  ////////////////////////////////////////////////////////////////////////////////

  glUseProgram(ctx.render_program);

  glGenVertexArrays(1, &ctx.render_vao);
  glGenBuffers(1, &ctx.render_vbo);
  glBindVertexArray(ctx.render_vao);
  glBindBuffer(GL_ARRAY_BUFFER, ctx.render_vbo);

  {
    const GLsizei stride = DISPLAY_RENDER_STRIDE * sizeof(F32);
    glVertexAttribPointer(
        0,                            // index
        2,                            // size
        GL_FLOAT,                     // type
        GL_FALSE,                     // normalized
        stride,                       // stride
        (Void*)0                      // offset
        );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1,                            // index
        2,                            // size
        GL_FLOAT,                     // type
        GL_FALSE,                     // normalized
        stride,                       // stride
        (Void*)(2 * sizeof(F32))      // offset
        );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        2,                            // index
        4,                            // size
        GL_UNSIGNED_BYTE,             // type
        GL_TRUE,                      // normalized
        stride,                       // stride
        (Void*)(4 * sizeof(F32))      // offset
        );
    glEnableVertexAttribArray(2);
  }

  ctx.projection = glGetUniformLocation(ctx.render_program, "projection");
  const M4F ortho = HMM_Orthographic_RH_NO(
      0.f,                        // left
      (F32) render.x,             // right
      (F32) render.y,             // bottom
      0.f,                        // top
      -1.f,                       // near
      1.f                         // far
      );
  glUniformMatrix4fv(ctx.projection, 1, GL_FALSE, (GLfloat*) &ortho.Elements);

  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  // framebuffer setup

  glGenFramebuffers(1, &ctx.fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, ctx.fbo);

  glGenTextures(1, &ctx.fb_color);
  glBindTexture(GL_TEXTURE_2D, ctx.fb_color);

  glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA,
      render.x, render.y,
      0,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      NULL
      );

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, DISPLAY_FB_FILTER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, DISPLAY_FB_FILTER);  

  glFramebufferTexture2D(
      GL_FRAMEBUFFER,
      GL_COLOR_ATTACHMENT0,
      GL_TEXTURE_2D,
      ctx.fb_color,
      0
      );
}

Void display_begin_frame()
{
  glBindFramebuffer(GL_FRAMEBUFFER, ctx.fbo);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(ctx.render_program);
  glViewport(0, 0, ctx.render_resolution.x, ctx.render_resolution.y);
  glBindVertexArray(ctx.render_vao);
  glEnable(GL_BLEND);
}

Void display_end_frame()
{
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(ctx.resample_program);
  glViewport(0, 0, ctx.window_resolution.x, ctx.window_resolution.y);
  glDisable(GL_BLEND);
  glBindVertexArray(ctx.resample_vao);
  glBindTexture(GL_TEXTURE_2D, ctx.fb_color);
  glDrawArrays(GL_TRIANGLES, 0, 6);
}

U32 display_color(U8 r, U8 g, U8 b, U8 a)
{
  const U32 or = r <<  0;
  const U32 og = g <<  8;
  const U32 ob = b << 16;
  const U32 oa = a << 24;
  return or | og | ob | oa;
}

U32 display_color_lerp(U32 a, U32 b, F32 t)
{
  const U8 ar = (U8) (a <<  0);
  const U8 ag = (U8) (a <<  8);
  const U8 ab = (U8) (a << 16);
  const U8 aa = (U8) (a << 24);

  const U8 br = (U8) (b <<  0);
  const U8 bg = (U8) (b <<  8);
  const U8 bb = (U8) (b << 16);
  const U8 ba = (U8) (b << 24);

  const U8 or = u8_lerp(ar, br, t);
  const U8 og = u8_lerp(ag, bg, t);
  const U8 ob = u8_lerp(ab, bb, t);
  const U8 oa = u8_lerp(aa, ba, t);

  return display_color(or, og, ob, oa);
}

Void display_begin_draw(TextureID texture)
{
  glBindTexture(GL_TEXTURE_2D, texture);
  display_sprite_index = 0;
}

Void display_end_draw()
{
  Vertex* const vertices = display_vertex_buffer;

  for (S32 i = 0; i < display_sprite_index; i++) {

    const S32 vi = DISPLAY_SPRITE_VERTICES * i;
    const Sprite* const sprite = &display_sprite_buffer[i];

    // top left
    vertices[vi + 0].x = sprite->root.x;
    vertices[vi + 0].y = sprite->root.y;
    vertices[vi + 0].u = sprite->ta.u;
    vertices[vi + 0].v = sprite->ta.v;
    vertices[vi + 0].color = sprite->color;

    // bottom left
    vertices[vi + 1].x = sprite->root.x;
    vertices[vi + 1].y = sprite->root.y + sprite->size.y;
    vertices[vi + 1].u = sprite->ta.u;
    vertices[vi + 1].v = sprite->tb.v;
    vertices[vi + 1].color = sprite->color;

    // bottom right
    vertices[vi + 2].x = sprite->root.x + sprite->size.x;
    vertices[vi + 2].y = sprite->root.y + sprite->size.y;
    vertices[vi + 2].u = sprite->tb.u;
    vertices[vi + 2].v = sprite->tb.v;
    vertices[vi + 2].color = sprite->color;

    // bottom right
    vertices[vi + 3].x = sprite->root.x + sprite->size.x;
    vertices[vi + 3].y = sprite->root.y + sprite->size.y;
    vertices[vi + 3].u = sprite->tb.u;
    vertices[vi + 3].v = sprite->tb.v;
    vertices[vi + 3].color = sprite->color;

    // top right
    vertices[vi + 4].x = sprite->root.x + sprite->size.x;
    vertices[vi + 4].y = sprite->root.y;
    vertices[vi + 4].u = sprite->tb.u;
    vertices[vi + 4].v = sprite->ta.v;
    vertices[vi + 4].color = sprite->color;

    // top left
    vertices[vi + 5].x = sprite->root.x;
    vertices[vi + 5].y = sprite->root.y;
    vertices[vi + 5].u = sprite->ta.u;
    vertices[vi + 5].v = sprite->ta.v;
    vertices[vi + 5].color = sprite->color;

  }

  const S32 vertex_count = DISPLAY_SPRITE_VERTICES * display_sprite_index;
  const GLsizeiptr size = vertex_count * sizeof(Vertex);
  glBindBuffer(GL_ARRAY_BUFFER, ctx.render_vbo);
  glBufferData(
      GL_ARRAY_BUFFER,    // target
      size,               // size in bytes
      vertices,           // data
      GL_DYNAMIC_DRAW     // usage
      );

  glBindVertexArray(ctx.render_vao);
  glDrawArrays(GL_TRIANGLES, 0, vertex_count);
}

Void display_draw_sprite(V2F root, V2F size, U32 color, V2F t1, V2F t2)
{
  Sprite sprite;
  sprite.ta     = t1;
  sprite.tb     = t2;
  sprite.color  = color;
  sprite.root   = root;
  sprite.size   = size;
  display_draw_sprite_struct(sprite);
}

Void display_draw_sprite_struct(Sprite sprite)
{
  if (display_sprite_index < DISPLAY_SPRITES) {
    display_sprite_buffer[display_sprite_index] = sprite;
    display_sprite_index += 1;
  }
}

Void display_draw_texture(V2F root, V2F size, U32 color)
{
  Sprite sprite;
  sprite.root   = root;
  sprite.size   = size;
  sprite.color  = color;
  display_draw_texture_struct(sprite);
}

Void display_draw_texture_struct(Sprite sprite)
{
  sprite.ta = v2f(0.f, 0.f);
  sprite.tb = v2f(1.f, 1.f);
  display_draw_sprite_struct(sprite);
}
