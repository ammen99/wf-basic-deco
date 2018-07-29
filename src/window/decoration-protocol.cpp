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
    init_frame_for_window(window);

    window->resize_request = [=] (int width, int height)
    {
        window->resize(width, height);

        auto m = frame_get_shadow_margin(window);
        xdg_surface_set_window_geometry(window->xsurface, m.left, m.top,
                                        window->width  - m.left - m.right,
                                        window->height - m.top  - m.bottom);
        paint_frame(window);
        window->damage_commit();
    };

    window->close_request = [=] ()
    {
        delete window;
    };


    view_to_decor[view] = window;
}

void view_resized(void *data,
                  struct wf_decorator_manager *wf_decorator_manager,
                  uint32_t view,
                  int32_t width,
                  int32_t height)
{
    auto window = view_to_decor[view];
    window->resize_request(width, height);
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
