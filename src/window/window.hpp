#ifndef COMMON_HPP
#define COMMON_HPP

#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"
#include "decoration-protocol.hpp"
#include <iostream>
#include <functional>
#include <vector>
#include <map>
#include <cairo.h>

struct wayfire_output;
struct wl_cursor;
struct wayfire_display
{
    wl_compositor *compositor = nullptr;
    wl_display    *display = nullptr;
    wl_shm        *shm = nullptr;
    wl_seat       *seat = nullptr;

    xdg_wm_base *wm_base = nullptr;

    wayfire_display();
    ~wayfire_display();

    wl_cursor *cursor = nullptr;
    wl_surface *cursor_surface = nullptr;

    bool load_cursor();
    void show_default_cursor(uint32_t serial);

    wf_decorator_manager *decorator_manager;
};

struct wf_shm_pool;
struct wayfire_window
{
	wl_surface   *surface   = nullptr;
    xdg_surface  *xsurface  = nullptr;
    xdg_toplevel *xtoplevel = nullptr;
    int pending_width = 0, pending_height = 0;

    int scale = 1;
    void set_scale(int scale);

    std::function<void(wl_pointer*, uint32_t, int x, int y)> pointer_enter;
    std::function<void()> pointer_leave;
    std::function<void(int x, int y)> pointer_move;
    std::function<void(uint32_t button, uint32_t state, int x, int y)> pointer_button;

    std::function<void(uint32_t time, int32_t id, uint32_t x, uint32_t y)> touch_down;
    std::function<void(int32_t id, uint32_t x, uint32_t y)> touch_motion;
    std::function<void(int32_t id)> touch_up;

    int width = 0, height = 0;
    wayfire_display *display;
    wf_shm_pool *pool;

    void *frame_data;
    cairo_surface_t *cairo_surface;

    std::function<void(int, int)> resize_request;
    std::function<void()> close_request;

    bool has_pointer_focus = false;

    wayfire_window(wayfire_display *display, std::string title);
    ~wayfire_window();

    void resize(int width, int height);
    void damage_commit();
};

/* the focused windows */
extern wayfire_window *current_touch_window, *current_pointer_window;

#endif /* end of include guard: COMMON_HPP */
