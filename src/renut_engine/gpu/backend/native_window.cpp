// Ported from github.com/ChanseyIsTheBest/futari-nx (pull request 2, commit 7f5a36a), app/src/renut_engine/gpu/backend/native_window.cpp. GPL-3.0 (LICENSE-GPL-3.0).
/**
 * @file    gpu/backend/native_window.cpp
 */
#if defined(ESPGALUDA2_GTK_WINDOW)
// espgaluda2-recomp: before plume's headers, which #undef Xlib's Status macro (plume_render_interface_types.h) that
// gdkx.h's Xutil.h needs.
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#endif
#include "renut_engine/gpu/backend/native_window.h"

#include "renut_engine/gpu/log.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__SWITCH__)
#include <switch.h>
#elif defined(ESPGALUDA2_GTK_WINDOW)
// espgaluda2-recomp: ReXGlue 0.8.0's Linux window is GTK (window_gtk.cpp), not SDL as in futari-nx's SDK.
#include <mutex>

#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#else
#include <cstdlib>

#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>
#endif

namespace renut::gpu {

#if defined(_WIN32)

namespace {

// The process's visible top-level window, preferring SDL's window class (the
// SDK creates one window, through SDL).
BOOL CALLBACK FindGameWindow(HWND hwnd, LPARAM param) {
  DWORD pid = 0;
  GetWindowThreadProcessId(hwnd, &pid);
  if (pid != GetCurrentProcessId() || !IsWindowVisible(hwnd) || GetWindow(hwnd, GW_OWNER))
    return TRUE;
  auto *found = reinterpret_cast<HWND *>(param);
  wchar_t class_name[32] = {};
  GetClassNameW(hwnd, class_name, 32);
  if (wcscmp(class_name, L"SDL_app") == 0) {
    *found = hwnd;
    return FALSE;
  }
  if (!*found)
    *found = hwnd;
  return TRUE;
}

HWND FindGameHwnd() {
  HWND hwnd = nullptr;
  EnumWindows(FindGameWindow, reinterpret_cast<LPARAM>(&hwnd));
  return hwnd;
}

} // namespace

bool GetNativeRenderWindow(plume::RenderWindow &out) {
  out = static_cast<plume::RenderWindow>(FindGameHwnd());
  if (!out) {
    RGPU_ERROR("No game window yet");
    return false;
  }
  return true;
}

bool GetRenderWindowPixelSize(uint32_t &width, uint32_t &height) {
  const HWND hwnd = FindGameHwnd();
  RECT rect{};
  if (!hwnd || !GetClientRect(hwnd, &rect))
    return false;
  width = uint32_t(rect.right - rect.left);
  height = uint32_t(rect.bottom - rect.top);
  return true;
}

#elif defined(__SWITCH__)

// banjo-nx: the Switch has one window, libnx's default NWindow, which plume's VK_NN_vi_surface takes.
bool GetNativeRenderWindow(plume::RenderWindow &out) {
  out = nwindowGetDefault();
  if (!out) {
    RGPU_ERROR("No default NWindow");
    return false;
  }
  return true;
}

bool GetRenderWindowPixelSize(uint32_t &width, uint32_t &height) {
  NWindow *window = nwindowGetDefault();
  u32 w = 0, h = 0;
  if (!window || R_FAILED(nwindowGetDimensions(window, &w, &h)) || !w || !h)
    return false;
  width = w;
  height = h;
  return true;
}

#elif defined(ESPGALUDA2_GTK_WINDOW)

// espgaluda2-recomp (original code, WORKLOG.md "Original code"): the GTK window of ReXGlue 0.8.0. The surface is the
// X11 window of the window's GtkDrawingArea, the same window the SDK's own Vulkan surface uses
// (GTKWindow::CreateSurfaceImpl: gdk_x11_display_get_xdisplay / gdk_x11_window_get_xid of drawing_area_). GTK is
// only used on the UI thread (StartRenderer); the handles are kept, and the size is asked through a private Xlib
// connection, which any thread may use under its mutex.
namespace {

std::mutex g_window_mutex;
Display *g_display = nullptr;  // GDK's connection (handed to plume for the surface)
Window g_xid = 0;
Display *g_size_display = nullptr;  // own connection for GetRenderWindowPixelSize

GtkWidget *FindDrawingArea(GtkWidget *widget) {
  if (GTK_IS_DRAWING_AREA(widget))
    return widget;
  if (!GTK_IS_CONTAINER(widget))
    return nullptr;
  GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
  GtkWidget *found = nullptr;
  for (GList *it = children; it && !found; it = it->next)
    found = FindDrawingArea(GTK_WIDGET(it->data));
  g_list_free(children);
  return found;
}

bool FindGameX11Window() {
  if (g_xid)
    return true;
  GList *toplevels = gtk_window_list_toplevels();
  for (GList *it = toplevels; it && !g_xid; it = it->next) {
    GtkWidget *area = FindDrawingArea(GTK_WIDGET(it->data));
    GdkWindow *gdk_window = area ? gtk_widget_get_window(area) : nullptr;
    GdkDisplay *gdk_display = area ? gtk_widget_get_display(area) : nullptr;
    if (!gdk_window || !gdk_display || !GDK_IS_X11_DISPLAY(gdk_display))
      continue;
    g_display = gdk_x11_display_get_xdisplay(gdk_display);
    g_xid = gdk_x11_window_get_xid(gdk_window);
  }
  g_list_free(toplevels);
  return g_xid != 0;
}

} // namespace

bool GetNativeRenderWindow(plume::RenderWindow &out) {
  std::lock_guard lock(g_window_mutex);
  if (!FindGameX11Window()) {
    RGPU_ERROR("No X11 GtkDrawingArea window for the Vulkan surface (GDK must use its X11 backend)");
    return false;
  }
  out.display = g_display;
  out.window = g_xid;
  return true;
}

bool GetRenderWindowPixelSize(uint32_t &width, uint32_t &height) {
  std::lock_guard lock(g_window_mutex);
  if (!g_xid)
    return false;
  if (!g_size_display)
    g_size_display = XOpenDisplay(DisplayString(g_display));
  XWindowAttributes attributes{};
  if (!g_size_display || !XGetWindowAttributes(g_size_display, g_xid, &attributes) || attributes.width <= 0 ||
      attributes.height <= 0)
    return false;
  width = uint32_t(attributes.width);
  height = uint32_t(attributes.height);
  return true;
}

#else

namespace {

// Runs before main, so before the SDK initialises SDL. plume builds its Vulkan
// surface with VK_KHR_xlib_surface, which needs the X11 video driver (XWayland
// on Wayland sessions). An explicit user setting is left alone.
[[maybe_unused]] const bool kPreferX11 = [] {
  setenv("SDL_VIDEO_DRIVER", "x11", 0);
  return true;
}();

// The app owns one window.
SDL_Window *FindGameSdlWindow() {
  int count = 0;
  SDL_Window **windows = SDL_GetWindows(&count);
  SDL_Window *sdl_window = (windows && count > 0) ? windows[0] : nullptr;
  SDL_free(windows);
  return sdl_window;
}

} // namespace

bool GetNativeRenderWindow(plume::RenderWindow &out) {
  SDL_Window *sdl_window = FindGameSdlWindow();
  if (!sdl_window) {
    RGPU_ERROR("No SDL window for the Vulkan surface");
    return false;
  }

  const SDL_PropertiesID props = SDL_GetWindowProperties(sdl_window);
  out.display = static_cast<Display *>(
      SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr));
  out.window = static_cast<Window>(
      SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
  if (!out.display || !out.window) {
    RGPU_ERROR("SDL window has no X11 handles (video driver '{}'); the Vulkan "
               "surface needs SDL_VIDEO_DRIVER=x11",
               SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "none");
    return false;
  }
  return true;
}

bool GetRenderWindowPixelSize(uint32_t &width, uint32_t &height) {
  SDL_Window *sdl_window = FindGameSdlWindow();
  int w = 0, h = 0;
  if (!sdl_window || !SDL_GetWindowSizeInPixels(sdl_window, &w, &h))
    return false;
  width = uint32_t(w);
  height = uint32_t(h);
  return true;
}

#endif

} // namespace renut::gpu
