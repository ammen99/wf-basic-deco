#ifndef FRAME_HPP
#define FRAME_HPP

#include "window.hpp"

struct frame_margin
{
    int top, bottom, left, right;
};

void init_frame_for_window(wayfire_window *window);
void fini_frame_for_window(wayfire_window *window);

void paint_frame(wayfire_window *window);

frame_margin frame_get_margin(wayfire_window *window);
frame_margin frame_get_shadow_margin(wayfire_window *window);

#endif /* end of include guard: FRAME_HPP */
