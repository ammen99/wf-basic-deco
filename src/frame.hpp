#ifndef FRAME_HPP
#define FRAME_HPP

#include "window.hpp"

void init_frame_for_window(wayfire_window *window);
void paint_frame(wayfire_window *window);
void frame_get_shadow_margin(wayfire_window *window, int& top, int& bottom, int& left, int& right);
void frame_get_margin(wayfire_window *window, int& top, int& bottom, int& left, int& right);

#endif /* end of include guard: FRAME_HPP */
