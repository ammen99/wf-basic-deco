#include "frame.hpp"
#include <cairo.h>

void init_frame_for_window(wayfire_window *)
{
}

void paint_frame(wayfire_window *window)
{
    auto cr = cairo_create(window->cairo_surface);

    cairo_set_source_rgb(cr, 1, 1, 0);
    cairo_rectangle(cr, 0, 0, window->width, window->height);
    cairo_fill(cr);
}

void frame_get_shadow_margin(wayfire_window *, int& top, int& bottom, int& left, int& right)
{
    top = 0;
    bottom = 0;
    left = 0;
    right = 0;
}

void frame_get_margin(wayfire_window *, int& top, int& bottom, int& left, int& right)
{
    top = bottom = left = right = 0;
}
