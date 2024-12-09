/*******************************************************************************
 * shell.c - program entry point
 ******************************************************************************/

#include "windows/wrapper.h"
#include <windowsx.h>

#ifdef PLATFORM_AUDIO
#include <audioclient.h>
#include <avrt.h>
#include <mmdeviceapi.h>
#endif

#include <string.h>
#include <stdatomic.h>
#include "loop.h"
#include "audio_format.h"
#include "log.h"

#define GLAD_GL_IMPLEMENTATION
#define GLAD_WGL_IMPLEMENTATION
#include "glad/wgl.h"

#define SHELL_CLASS_NAME "SHELL_WINDOW_CLASS"
#define SHELL_EXIT_SUCCESS 0
#define SHELL_EXIT_FAILURE 1
#define SHELL_GL_VERSION_MAJOR 4
#define SHELL_GL_VERSION_MINOR 5
#define SHELL_AUDIO_TIMEOUT 2000

#define VK_CARDINAL 0x100

#define THREAD_EXIT_SUCCESS 0
#define THREAD_EXIT_FAILURE 1

#ifdef PLATFORM_AUDIO

typedef struct AudioDevice {
  IMMDevice* device;
  IAudioClient* client;
  IAudioRenderClient* render;
  HANDLE event;
  UINT32 buffer_size;
} AudioDevice;

typedef struct AudioBuffer {
  Index frames;
  F32* data;
} AudioBuffer;

#endif

/*******************************************************************************
 * STATIC DATA
 ******************************************************************************/

static HWND shell_window = NULL;

// signal for audio thread
static _Atomic Bool quit_signal = false;

static KeyCode shell_key_table[VK_CARDINAL] = {
  [ VK_LBUTTON    ] = KEYCODE_MOUSE_LEFT,
  [ VK_RBUTTON    ] = KEYCODE_MOUSE_RIGHT,
  [ VK_MBUTTON    ] = KEYCODE_MOUSE_MIDDLE,
  [ VK_BACK       ] = KEYCODE_BACKSPACE,
  [ VK_TAB        ] = KEYCODE_TAB,
  [ VK_RETURN     ] = KEYCODE_ENTER,
  [ VK_SHIFT      ] = KEYCODE_SHIFT,
  [ VK_CONTROL    ] = KEYCODE_CONTROL,
  [ VK_MENU       ] = KEYCODE_ALT,
  [ VK_CAPITAL    ] = KEYCODE_CAPS,
  [ VK_ESCAPE     ] = KEYCODE_ESCAPE,
  [ VK_SPACE      ] = KEYCODE_SPACE,
  [ VK_PRIOR      ] = KEYCODE_PAGE_UP,
  [ VK_NEXT       ] = KEYCODE_PAGE_DOWN,
  [ VK_END        ] = KEYCODE_END,
  [ VK_HOME       ] = KEYCODE_HOME,
  [ VK_INSERT     ] = KEYCODE_INSERT,
  [ VK_DELETE     ] = KEYCODE_DELETE,
  [ VK_LEFT       ] = KEYCODE_ARROW_LEFT,
  [ VK_RIGHT      ] = KEYCODE_ARROW_RIGHT,
  [ VK_UP         ] = KEYCODE_ARROW_UP,
  [ VK_DOWN       ] = KEYCODE_ARROW_DOWN,
  [ VK_F1         ] = KEYCODE_F1,
  [ VK_F2         ] = KEYCODE_F2,
  [ VK_F3         ] = KEYCODE_F3,
  [ VK_F4         ] = KEYCODE_F4,
  [ VK_F5         ] = KEYCODE_F5,
  [ VK_F6         ] = KEYCODE_F6,
  [ VK_F7         ] = KEYCODE_F7,
  [ VK_F8         ] = KEYCODE_F8,
  [ VK_F9         ] = KEYCODE_F9,
  [ VK_F10        ] = KEYCODE_F10,
  [ VK_F11        ] = KEYCODE_F11,
  [ VK_F12        ] = KEYCODE_F12,
  [ '0'           ] = KEYCODE_0,
  [ '1'           ] = KEYCODE_1,
  [ '2'           ] = KEYCODE_2,
  [ '3'           ] = KEYCODE_3,
  [ '4'           ] = KEYCODE_4,
  [ '5'           ] = KEYCODE_5,
  [ '6'           ] = KEYCODE_6,
  [ '7'           ] = KEYCODE_7,
  [ '8'           ] = KEYCODE_8,
  [ '9'           ] = KEYCODE_9,
  [ 'A'           ] = KEYCODE_A,
  [ 'B'           ] = KEYCODE_B,
  [ 'C'           ] = KEYCODE_C,
  [ 'D'           ] = KEYCODE_D,
  [ 'E'           ] = KEYCODE_E,
  [ 'F'           ] = KEYCODE_F,
  [ 'G'           ] = KEYCODE_G,
  [ 'H'           ] = KEYCODE_H,
  [ 'I'           ] = KEYCODE_I,
  [ 'J'           ] = KEYCODE_J,
  [ 'K'           ] = KEYCODE_K,
  [ 'L'           ] = KEYCODE_L,
  [ 'M'           ] = KEYCODE_M,
  [ 'N'           ] = KEYCODE_N,
  [ 'O'           ] = KEYCODE_O,
  [ 'P'           ] = KEYCODE_P,
  [ 'Q'           ] = KEYCODE_Q,
  [ 'R'           ] = KEYCODE_R,
  [ 'S'           ] = KEYCODE_S,
  [ 'T'           ] = KEYCODE_T,
  [ 'U'           ] = KEYCODE_U,
  [ 'V'           ] = KEYCODE_V,
  [ 'W'           ] = KEYCODE_W,
  [ 'X'           ] = KEYCODE_X,
  [ 'Y'           ] = KEYCODE_Y,
  [ 'Z'           ] = KEYCODE_Z,
  [ VK_OEM_3      ] = KEYCODE_GRAVE,
  [ VK_OEM_PLUS   ] = KEYCODE_PLUS,
  [ VK_OEM_MINUS  ] = KEYCODE_MINUS,
  [ VK_OEM_4      ] = KEYCODE_BRACKET_OPEN,
  [ VK_OEM_6      ] = KEYCODE_BRACKET_CLOSE,
  [ VK_OEM_5      ] = KEYCODE_BACKSLASH,
  [ VK_OEM_1      ] = KEYCODE_SEMICOLON,
  [ VK_OEM_7      ] = KEYCODE_APOSTROPHE,
  [ VK_OEM_COMMA  ] = KEYCODE_COMMA,
  [ VK_OEM_PERIOD ] = KEYCODE_PERIOD,
  [ VK_OEM_2      ] = KEYCODE_SLASH,
};

/*******************************************************************************
 * AUDIO CALLBACKS
 ******************************************************************************/

#ifdef PLATFORM_AUDIO

static Bool shell_audio_acquire_buffer(AudioBuffer* out, const AudioDevice* device)
{
  const DWORD wait_status = WaitForSingleObject(device->event, SHELL_AUDIO_TIMEOUT);
  if (wait_status != WAIT_OBJECT_0) {
    return true;
  }

  UINT32 padding = 0;
  const HRESULT padding_status = IAudioClient_GetCurrentPadding(device->client, &padding);
  if (FAILED(padding_status)) {
    return true;
  }

  const UINT32 frames = device->buffer_size - padding;
  const HRESULT buffer_status = IAudioRenderClient_GetBuffer(
      device->render,
      frames,
      (BYTE**) &out->data
      );
  if (SUCCEEDED(buffer_status)) {
    out->frames = frames;
    return false;
  } else {
    return true;
  }
}

static DWORD WINAPI shell_audio_entry(Void* data)
{
  UNUSED_PARAMETER(data);
  HRESULT hr;
  IMMDeviceEnumerator* enumerator = NULL;
  AudioDevice device = {0};

  hr = CoInitializeEx(NULL, COINIT_SPEED_OVER_MEMORY | COINIT_MULTITHREADED);
  if (FAILED(hr)) {
    platform_log_error("failed to initialize COM");
    goto cleanup;
  }

  DWORD task_index = 0;
  const HANDLE task_handle = AvSetMmThreadCharacteristicsA("Pro Audio", &task_index);
  if (task_handle == 0) {
    platform_log_warn("failed to set pro audio thread characteristic");
  }

  hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, &IID_IMMDeviceEnumerator, &enumerator);
  if (FAILED(hr)) {
    platform_log_error("failed to create audio device enumerator");
    goto cleanup;
  }

  hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device.device);
  if (FAILED(hr)) {
    platform_log_error("failed to get default audio endpoint");
    goto cleanup;
  }

  hr = IMMDevice_Activate(device.device, &IID_IAudioClient, CLSCTX_ALL, NULL, &device.client);
  if (FAILED(hr)) {
    platform_log_error("failed to activate audio device");
    goto cleanup;
  }

  REFERENCE_TIME duration = 0;
  hr = IAudioClient_GetDevicePeriod(device.client, &duration, NULL);
  if (FAILED(hr)) {
    platform_log_error("failed to get audio device period");
    goto cleanup;
  }

  const WORD block_align = STEREO * PLATFORM_BIT_DEPTH / 8;
  const DWORD throughput = PLATFORM_SAMPLE_RATE * block_align;
  WAVEFORMATEXTENSIBLE audio_format;
  audio_format.Format.cbSize                = sizeof(audio_format);
  audio_format.Format.wFormatTag            = WAVE_FORMAT_EXTENSIBLE;
  audio_format.Format.wBitsPerSample        = PLATFORM_BIT_DEPTH;
  audio_format.Format.nChannels             = STEREO;
  audio_format.Format.nSamplesPerSec        = PLATFORM_SAMPLE_RATE;
  audio_format.Format.nBlockAlign           = block_align;
  audio_format.Format.nAvgBytesPerSec       = throughput;
  audio_format.Samples.wValidBitsPerSample  = PLATFORM_BIT_DEPTH;
  audio_format.dwChannelMask                = KSAUDIO_SPEAKER_STEREO;
  audio_format.SubFormat                    = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
  WAVEFORMATEX* const format = &audio_format.Format;

  const DWORD audio_client_flags
    = AUDCLNT_STREAMFLAGS_NOPERSIST
    | AUDCLNT_STREAMFLAGS_EVENTCALLBACK
    | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
    | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY
    ;

  hr = IAudioClient_Initialize(
      device.client, 
      AUDCLNT_SHAREMODE_SHARED,
      audio_client_flags,
      duration,                   // buffer duration
      0,                          // periodicity
      format,
      NULL
      );
  if (FAILED(hr)) {
    platform_log_error("failed to initialize audio client");
    goto cleanup;
  }

  device.event = CreateEvent(NULL, FALSE, FALSE, NULL);
  if (device.event == NULL) {
    platform_log_error("failed to create wasapi event");
    goto cleanup;
  }

  hr = IAudioClient_SetEventHandle(device.client, device.event);
  if (FAILED(hr)) {
    platform_log_error("failed to set wasapi event handle");
    goto cleanup;
  }

  hr = IAudioClient_GetService(device.client, &IID_IAudioRenderClient, &device.render);
  if (FAILED(hr)) {
    platform_log_error("failed to get wasapi render service");
    goto cleanup;
  }

  hr = IAudioClient_GetBufferSize(device.client, &device.buffer_size);
  if (FAILED(hr)) {
    platform_log_error("failed to get wasapi buffer size");
    goto cleanup;
  }

  hr = IAudioClient_Start(device.client);
  if (FAILED(hr)) {
    platform_log_error("failed to start audio client");
    goto cleanup;
  }

  ProgramStatus status = PROGRAM_STATUS_LIVE;
  while (status == PROGRAM_STATUS_LIVE) {

    AudioBuffer buffer = {0};
    const Bool failed = shell_audio_acquire_buffer(&buffer, &device);

    if (failed == false) {
      status = loop_audio(buffer.data, buffer.frames);
      IAudioRenderClient_ReleaseBuffer(device.render, (U32) buffer.frames, 0);
    } else {
      status = PROGRAM_STATUS_FAILURE;
    }

    const Bool signal = atomic_load(&quit_signal);
    if (signal) {
      status = PROGRAM_STATUS_SUCCESS;
    }

  }

cleanup:

  if (device.event) {
    CloseHandle(device.event);
  }
  if (device.render) {
    IAudioRenderClient_Release(device.render);
  }
  if (device.client) {
    IAudioClient_Release(device.client);
  }
  if (device.device) {
    IMMDevice_Release(device.device);
  }
  if (enumerator) {
    IMMDeviceEnumerator_Release(enumerator);
  }
  return THREAD_EXIT_SUCCESS;

}

#endif

/*******************************************************************************
 * WINDOW MANAGEMENT
 ******************************************************************************/

V2S read_cursor()
{
  POINT mouse = {0};
  GetCursorPos(&mouse);
  ScreenToClient(shell_window, &mouse);
  const V2S out = { mouse.x, mouse.y };
  return out;
}

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{

  switch (msg) {

    case WM_DESTROY:
      {
        PostQuitMessage(0);
      } break;

    case WM_KEYDOWN:
      {
        const WORD vkcode = LOWORD(wparam);
        const KeyCode kc  = shell_key_table[vkcode];
#if 0
        const WORD flags  = HIWORD(lparam);
        const Bool repeat = (flags & KF_REPEAT) == KF_REPEAT;
#endif
        if (kc != KEYCODE_NONE) {
          const Event event = key_event(KEYSTATE_DOWN, kc);
          loop_event(&event);
        }
      } break;

    case WM_KEYUP:
      {
        const WORD vkcode = LOWORD(wparam);
        const KeyCode kc  = shell_key_table[vkcode];
        if (kc != KEYCODE_NONE) {
          const Event event = key_event(KEYSTATE_UP, kc);
          loop_event(&event);
        }
      } break;

    case WM_CHAR:
      {
        const Event event = character_event((Char) wparam);
        loop_event(&event);
      } break;

    case WM_LBUTTONDOWN:
      {
        const Event event = key_event(KEYSTATE_DOWN, KEYCODE_MOUSE_LEFT);
        loop_event(&event);
      } break;

    case WM_LBUTTONUP:
      {
        const Event event = key_event(KEYSTATE_UP, KEYCODE_MOUSE_LEFT);
        loop_event(&event);
      } break;

    case WM_RBUTTONDOWN:
      {
        const Event event = key_event(KEYSTATE_DOWN, KEYCODE_MOUSE_RIGHT);
        loop_event(&event);
      } break;

    case WM_RBUTTONUP:
      {
        const Event event = key_event(KEYSTATE_UP, KEYCODE_MOUSE_RIGHT);
        loop_event(&event);
      } break;

    case WM_MOUSEMOVE:
      {
        const S32 x = GET_X_LPARAM(lparam);
        const S32 y = GET_Y_LPARAM(lparam);
        const Event event = mouse_move_event(v2s(x, y));
        loop_event(&event);
      } break;

    default:
      return DefWindowProc(hwnd, msg, wparam, lparam);

  }

  return 0;

}

static INT shell_exit_code(ProgramStatus status)
{
  return status == PROGRAM_STATUS_FAILURE ? SHELL_EXIT_FAILURE : SHELL_EXIT_SUCCESS;
}

INT WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR cmdline, INT ncmdshow)
{

  UNUSED_PARAMETER(prev_instance);
  UNUSED_PARAMETER(cmdline);
  UNUSED_PARAMETER(ncmdshow);

  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);

  // Without this call, windows will show an annoying momentary loading cursor
  // when the user first mouses over the window.
  SetCursor(NULL);

  SystemInfo system;
  system.display.x = GetSystemMetrics(SM_CXSCREEN);
  system.display.y = GetSystemMetrics(SM_CYSCREEN);

  ProgramConfig config = {0};
  const ProgramStatus config_status = loop_config(&config, &system);
  if (config_status != PROGRAM_STATUS_LIVE) {
    return shell_exit_code(config_status);
  }

  if (config.normalize_working_directory) {

    // get executable file name
    Char exe_path[MAX_PATH] = {0};
    const U32 exe_length = GetModuleFileName(NULL, exe_path, MAX_PATH);
    if (exe_length == 0) {
      platform_log_error("failed to get executable filename");
      return SHELL_EXIT_FAILURE;
    }

    // get full path
    Char full_path[MAX_PATH] = {0};
    Char* file_part = NULL;
    const U32 path_length = GetFullPathName(exe_path, MAX_PATH, full_path, &file_part);
    if (path_length == 0) {
      platform_log_error("failed to get executable directory");
      return SHELL_EXIT_FAILURE;
    }

    // get parent
    Char exe_dir[MAX_PATH] = {0};
    strncpy(exe_dir, full_path, file_part - full_path);

    // set working directory
    const Bool cd_status = SetCurrentDirectory(exe_dir);
    if (cd_status == 0) {
      platform_log_error("failed to set working directory");
      return SHELL_EXIT_FAILURE;
    }

  }

  WNDCLASSEX wc = {0};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = wnd_proc;
  wc.hInstance = instance;
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.lpszClassName = SHELL_CLASS_NAME;

  const ATOM class_id = RegisterClassEx(&wc);
  if (class_id == 0) {
    platform_log_error("failed to register window class");
    return SHELL_EXIT_FAILURE;
  }

  RECT client_rect = {0};
  client_rect.left    = 0;
  client_rect.top     = 0;
  client_rect.right   = config.resolution.x;
  client_rect.bottom  = config.resolution.y;

  const DWORD style = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX;
  const DWORD exstyle = WS_EX_APPWINDOW;

  const BOOL adjust_result = AdjustWindowRectEx(&client_rect, style, FALSE, exstyle);
  if (adjust_result == FALSE) {
    platform_log_error("failed to adjust window client area");
    return SHELL_EXIT_FAILURE;
  }

  V2S window_size;
  window_size.x = client_rect.right - client_rect.left;
  window_size.y = client_rect.bottom - client_rect.top;

#if 0
  // @rdk: add a fullscreen option
  shell_window = CreateWindowEx(
      0,                              // optional window styles
      SHELL_CLASS_NAME,               // window class
      SHELL_TITLE,                    // window text
      WS_POPUP,   // window style
      0, 0,       // position
      primary_display.x + 0, primary_display.y + 0, // size
      NULL,       // parent window
      NULL,       // menu
      instance,   // instance handle
      NULL        // additional application data
      );
#endif

  shell_window = CreateWindowEx(
      exstyle,
      SHELL_CLASS_NAME,
      config.title,
      style,
      CW_USEDEFAULT,
      0,
      window_size.x,
      window_size.y,
      NULL,
      NULL,
      instance,
      NULL
      );

  if (shell_window == NULL) {
    platform_log_error("failed to create window");
    return SHELL_EXIT_FAILURE;
  }

  const HDC hdc = GetDC(shell_window);
  if (hdc == NULL) {
    platform_log_error("failed to get device context");
    return SHELL_EXIT_FAILURE;
  }

  PIXELFORMATDESCRIPTOR pfd = {0};
  pfd.nSize = sizeof(pfd);
  pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 32;
  pfd.cDepthBits = 32;
  pfd.iLayerType = PFD_MAIN_PLANE;

  const S32 format = ChoosePixelFormat(hdc, &pfd);
  if (format == 0) {
    platform_log_error("failed to choose pixel format");
    return SHELL_EXIT_FAILURE;
  }

  const BOOL set_pixel_format_result = SetPixelFormat(hdc, format, &pfd);
  if (set_pixel_format_result == FALSE) {
    platform_log_error("failed to set pixel format");
    return SHELL_EXIT_FAILURE;
  }

  const HGLRC bootstrap = wglCreateContext(hdc);
  if (bootstrap == NULL) {
    platform_log_error("failed to create bootstrap OpenGL context");
    return SHELL_EXIT_FAILURE;
  }

  const BOOL bootstrap_current = wglMakeCurrent(hdc, bootstrap);
  if (bootstrap_current == FALSE) {
    platform_log_error("failed to activate bootstrap OpenGL context");
    return SHELL_EXIT_FAILURE;
  }

  const S32 glad_wgl_version = gladLoaderLoadWGL(hdc);
  if (glad_wgl_version == 0) {
    platform_log_error("GLAD WGL loader failed");
    return SHELL_EXIT_FAILURE;
  }

  if (GLAD_WGL_ARB_create_context == 0) {
    platform_log_error("missing required extension: WGL_ARB_create_context");
    return SHELL_EXIT_FAILURE;
  }

  if (GLAD_WGL_ARB_create_context_profile == 0) {
    platform_log_error("missing required extension: WGL_ARB_create_context_profile");
    return SHELL_EXIT_FAILURE;
  }

  S32 wgl_attributes[] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB, SHELL_GL_VERSION_MAJOR,
    WGL_CONTEXT_MINOR_VERSION_ARB, SHELL_GL_VERSION_MINOR,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#if 0
    WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
    WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
    0 // end
  };

  const HGLRC glctx = wglCreateContextAttribsARB(hdc, NULL, wgl_attributes);
  if (glctx == NULL) {
    platform_log_error("failed to create OpenGL context");
    return SHELL_EXIT_FAILURE;
  }

  const BOOL clear_context = wglMakeCurrent(NULL, NULL);
  if (clear_context == FALSE) {
    platform_log_error("failed to deactivate bootstrap OpenGL context");
    return SHELL_EXIT_FAILURE;
  }

  const BOOL deleted_context = wglDeleteContext(bootstrap);
  if (deleted_context == FALSE) {
    platform_log_error("failed to delete OpenGL context");
    return SHELL_EXIT_FAILURE;
  }

  const BOOL glctx_current = wglMakeCurrent(hdc, glctx);
  if (glctx_current == FALSE) {
    platform_log_error("failed to activate OpenGL context");
    return SHELL_EXIT_FAILURE;
  }

  const S32 gl_version = gladLoaderLoadGL();
  if (gl_version == 0) {
    platform_log_error("GLAD loader failed");
    return SHELL_EXIT_FAILURE;
  }

  if (GLAD_WGL_EXT_swap_control) {
    const BOOL vsync_status = wglSwapIntervalEXT(1);
    if (vsync_status == FALSE) {
      platform_log_warn("failed to set vsync");
    }
  }

  const ProgramStatus init_status = loop_init();
  if (init_status != PROGRAM_STATUS_LIVE) {
    return shell_exit_code(init_status);
  }

  // We swap buffers before showing the window in case the client rendered a
  // frame during initialization.
  SwapBuffers(hdc);
  ShowWindow(shell_window, SW_SHOW);

#ifdef PLATFORM_AUDIO

  DWORD audio_thread_id = 0;
  const HANDLE audio_thread = CreateThread( 
      NULL,               // default security attributes
      0,                  // default stack size
      shell_audio_entry,  // entry point
      NULL,
      0,                  // default creation flags
      &audio_thread_id    // receive thread identifier
      );
  if (audio_thread == NULL) {
    platform_log_error("failed to start audio thread");
  } else {
    const BOOL priority_status = SetThreadPriority(audio_thread, THREAD_PRIORITY_HIGHEST);
    if (priority_status == FALSE) {
      platform_log_warn("failed to set audio thread priority");
    }
  }

#endif

  ProgramStatus status = PROGRAM_STATUS_LIVE;
  Bool quit = false;
  while (quit == false && status == PROGRAM_STATUS_LIVE) {

    MSG msg = {0};
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        quit = true;
      }
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    if (quit == false) {
      status = loop_video();
      SwapBuffers(hdc);
    }

  }

  loop_terminate();

#ifdef PLATFORM_AUDIO

  // signal the audio thread and wait
  atomic_store(&quit_signal, true);
  const DWORD wait_status = WaitForSingleObject(audio_thread, INFINITE);
  if (wait_status != WAIT_OBJECT_0) {
    platform_log_warn("unexpected wait status for audio thread");
  }

#endif

  return SHELL_EXIT_SUCCESS;

}
