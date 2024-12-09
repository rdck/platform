/*******************************************************************************
 * event.h - platform events
 ******************************************************************************/

#pragma once

#include "prelude.h"

typedef enum {
  KEYCODE_NONE = 0,
  KEYCODE_MOUSE_LEFT,
  KEYCODE_MOUSE_RIGHT,
  KEYCODE_MOUSE_MIDDLE,
  KEYCODE_BACKSPACE,
  KEYCODE_TAB,
  KEYCODE_ENTER,
  KEYCODE_SHIFT,
  KEYCODE_CONTROL,
  KEYCODE_ALT,
  KEYCODE_CAPS,
  KEYCODE_ESCAPE,
  KEYCODE_SPACE,
  KEYCODE_PAGE_UP,
  KEYCODE_PAGE_DOWN,
  KEYCODE_END,
  KEYCODE_HOME,
  KEYCODE_INSERT,
  KEYCODE_DELETE,
  KEYCODE_ARROW_LEFT,
  KEYCODE_ARROW_RIGHT,
  KEYCODE_ARROW_UP,
  KEYCODE_ARROW_DOWN,
  KEYCODE_F1,
  KEYCODE_F2,
  KEYCODE_F3,
  KEYCODE_F4,
  KEYCODE_F5,
  KEYCODE_F6,
  KEYCODE_F7,
  KEYCODE_F8,
  KEYCODE_F9,
  KEYCODE_F10,
  KEYCODE_F11,
  KEYCODE_F12,
  KEYCODE_0,
  KEYCODE_1,
  KEYCODE_2,
  KEYCODE_3,
  KEYCODE_4,
  KEYCODE_5,
  KEYCODE_6,
  KEYCODE_7,
  KEYCODE_8,
  KEYCODE_9,
  KEYCODE_A,
  KEYCODE_B,
  KEYCODE_C,
  KEYCODE_D,
  KEYCODE_E,
  KEYCODE_F,
  KEYCODE_G,
  KEYCODE_H,
  KEYCODE_I,
  KEYCODE_J,
  KEYCODE_K,
  KEYCODE_L,
  KEYCODE_M,
  KEYCODE_N,
  KEYCODE_O,
  KEYCODE_P,
  KEYCODE_Q,
  KEYCODE_R,
  KEYCODE_S,
  KEYCODE_T,
  KEYCODE_U,
  KEYCODE_V,
  KEYCODE_W,
  KEYCODE_X,
  KEYCODE_Y,
  KEYCODE_Z,
  KEYCODE_GRAVE,
  KEYCODE_PLUS,
  KEYCODE_MINUS,
  KEYCODE_BRACKET_OPEN,
  KEYCODE_BRACKET_CLOSE,
  KEYCODE_BACKSLASH,
  KEYCODE_SEMICOLON,
  KEYCODE_APOSTROPHE,
  KEYCODE_COMMA,
  KEYCODE_PERIOD,
  KEYCODE_SLASH,
  KEYCODE_CARDINAL,
} KeyCode;

typedef enum {
  KEYSTATE_UP = 0,
  KEYSTATE_DOWN,
  KEYSTATE_CARDINAL,
} KeyState;

typedef struct {
  KeyState state;
  KeyCode code;
} KeyEvent;

typedef struct {
  Char character;
} CharacterEvent;

typedef enum EventTag {
  EVENT_NONE,
  EVENT_KEY,
  EVENT_CHARACTER,
  EVENT_MOUSE,
  EVENT_CARDINAL,
} EventTag;

typedef struct Event {
  EventTag tag;
  union {
    KeyEvent key;
    CharacterEvent character;
    V2S mouse;
  };
} Event;

// @rdk: move to somewhere more sensible
V2S read_cursor();

static inline Event key_event(KeyState state, KeyCode code)
{
  Event out;
  out.tag = EVENT_KEY;
  out.key.state = state;
  out.key.code = code;
  return out;
}

static inline Event character_event(Char c)
{
  Event out;
  out.tag = EVENT_CHARACTER;
  out.character.character = c;
  return out;
}

static inline Event mouse_move_event(V2S mouse)
{
  Event out;
  out.tag = EVENT_MOUSE;
  out.mouse = mouse;
  return out;
}
