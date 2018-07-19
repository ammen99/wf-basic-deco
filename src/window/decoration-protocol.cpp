#include <map>
#include <iostream>

#include "decoration-protocol.hpp"
#include "window.hpp"
#include "../frame.hpp"


std::map<uint32_t, wayfire_window*> view_to_decor;
static void create_new_decoration(void *data, wf_decorator_manager *, uint32_t view)
{
    auto display = (wayfire_display*) data;

    std::cout << "create new decoration" << std::endl;
    std::string title = "__wf_decorator:" + std::to_string(view);

    auto window = new wayfire_window(display, title);
    window->resize_request = [=] (int width, int height)
    {
        window->resize(width, height);
        init_frame_for_window(window);

        int top, bottom, left, right;
        frame_get_shadow_margin(window, top, bottom, left, right);
        xdg_surface_set_window_geometry(window->xsurface, left, top,
                                        window->width - left - right,
                                        window->height - top - bottom);
        paint_frame(window);
        window->damage_commit();
    };

    window->close_request = [=] ()
    {
        // TODO
    };


    view_to_decor[view] = window;
}

void view_resized(void *data,
                  struct wf_decorator_manager *wf_decorator_manager,
                  uint32_t view,
                  int32_t width,
                  int32_t height)
{
    //auto window = view_to_decor[view];
    //gtk_window_resize(GTK_WINDOW(window), width, height);
}

static void title_changed(void *data,
                          wf_decorator_manager *wf_decorator_manager,
                          uint32_t view,
                          const char *new_title)
{
    //set_title(view_to_decor[view], new_title);
}


const struct wf_decorator_manager_listener wf_decorator_manager_impl = 
{
    create_new_decoration,
    view_resized,
    title_changed
};
