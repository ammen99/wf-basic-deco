#include "input.hpp"

wayfire_window* current_touch_window = nullptr, *current_pointer_window = nullptr;
static size_t current_window_touch_points = 0;
static int pointer_x, pointer_y;

void pointer_enter(void *, struct wl_pointer *wl_pointer,
    uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
    /* possibly an event for a surface we just destroyed */
    if (!surface)
        return;

    pointer_x = wl_fixed_to_int(surface_x);
    pointer_y = wl_fixed_to_int(surface_y);

    auto window = (wayfire_window*) wl_surface_get_user_data(surface);
    if (window && window->pointer_enter)
        window->pointer_enter(wl_pointer, serial,
                pointer_x * window->scale, pointer_y * window->scale);
    if (window)
    {
        current_pointer_window = window;
        window->has_pointer_focus = true;
    }
}

void pointer_leave(void *, struct wl_pointer *, uint32_t, struct wl_surface *surface)
{
    /* possibly an event for a surface we just destroyed */
    if (!surface)
        return;

    auto window = (wayfire_window*) wl_surface_get_user_data(surface);
    if (window && window->pointer_leave)
    {
        window->pointer_leave();
        window->has_pointer_focus = false;
    }

    current_pointer_window = nullptr;
}

void pointer_motion(void*, struct wl_pointer*, uint32_t, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
    pointer_x = wl_fixed_to_int(surface_x);
    pointer_y = wl_fixed_to_int(surface_y);
    if (current_pointer_window && current_pointer_window->pointer_move)
        current_pointer_window->pointer_move(pointer_x * current_pointer_window->scale,
                                             pointer_y * current_pointer_window->scale);
}

void pointer_button(void *, struct wl_pointer *, uint32_t, uint32_t, uint32_t button, uint32_t state)
{
    if (current_pointer_window && current_pointer_window->pointer_button)
        current_pointer_window->pointer_button(button, state,
                                               pointer_x * current_pointer_window->scale,
                                               pointer_y * current_pointer_window->scale);
}

void pointer_axis         (void*, struct wl_pointer*, uint32_t, uint32_t, wl_fixed_t) {}
void pointer_frame        (void*, struct wl_pointer*) {}
void pointer_axis_stop    (void*, struct wl_pointer*, uint32_t, uint32_t) {}
void pointer_axis_source  (void*, struct wl_pointer*, uint32_t) {}
void pointer_axis_discrete(void*, struct wl_pointer*, uint32_t, int32_t) {}

const struct wl_pointer_listener pointer_listener = {
    .enter = pointer_enter,
    .leave = pointer_leave,
    .motion = pointer_motion,
    .button = pointer_button,
    .axis = pointer_axis,
    .frame = pointer_frame,
    .axis_source = pointer_axis_source,
    .axis_stop = pointer_axis_stop,
    .axis_discrete = pointer_axis_discrete
};

static void touch_down(void *, struct wl_touch *,
                       uint32_t , uint32_t time, struct wl_surface *surface,
                       int32_t id, wl_fixed_t x, wl_fixed_t y)
{
    auto window = (wayfire_window*) wl_surface_get_user_data(surface);

    if (current_touch_window != window)
        current_window_touch_points = 0;

    current_touch_window = window;
    current_window_touch_points++;

    if (window->touch_down)
        window->touch_down(time, id,
                           wl_fixed_to_int(x) * window->scale,
                           wl_fixed_to_int(y) * window->scale);
}

static void
touch_up(void *, struct wl_touch *,
         uint32_t , uint32_t , int32_t id)
{
    if (current_touch_window && current_touch_window->touch_down)
        current_touch_window->touch_up(id);

    current_window_touch_points--;
    if (!current_window_touch_points)
        current_touch_window = nullptr;
}

static void
touch_motion(void *, struct wl_touch *,
             uint32_t , int32_t id, wl_fixed_t x, wl_fixed_t y)
{
    if (current_touch_window->touch_motion)
        current_touch_window->touch_motion(id,
                                           wl_fixed_to_int(x) * current_touch_window->scale,
                                           wl_fixed_to_int(y) * current_touch_window->scale);
}

static void
touch_frame(void *, wl_touch *) {}

static void
touch_cancel(void *, wl_touch *) {}

const struct wl_touch_listener touch_listener = {
    touch_down,
    touch_up,
    touch_motion,
    touch_frame,
    touch_cancel,
    NULL,
    NULL
};
