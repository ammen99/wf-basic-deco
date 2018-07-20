#include "frame.hpp"
#include <cairo.h>

struct frame_data
{
    frame_margin shadow, real;
};

void init_frame_for_window(wayfire_window *window)
{
    auto frame = new frame_data;
    frame->shadow = {0, 0, 0, 0};
    frame->real = {35, 1, 1, 1};

    window->frame_data = frame;
}

void fini_frame_for_window(wayfire_window *window)
{
    auto frame = (frame_data*) window->frame_data;
    delete frame;
}

void fill_rect(cairo_t *cr, int x, int y, int w, int h)
{
    cairo_set_source_rgba(cr, 1, 1, 0, 1);
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);
}

void paint_frame(wayfire_window *window)
{
    auto frame = (frame_data*) window->frame_data;
    fill_rect(window->cr, 0, 0, window->width, frame->real.top);
    fill_rect(window->cr, 0, 0, frame->real.left, window->height);
    fill_rect(window->cr, 0, window->height - frame->real.bottom, window->width, frame->real.bottom);
    fill_rect(window->cr, window->width - frame->real.right, 0, frame->real.right, window->height);
}

frame_margin frame_get_shadow_margin(wayfire_window *window)
{
    auto frame = (frame_data*) window->frame_data;
    return frame->shadow;
}

frame_margin frame_get_margin(wayfire_window *window)
{
    auto frame = (frame_data*) window->frame_data;
    return frame->real;
}
