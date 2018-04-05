#include "protocol.hpp"
#include "wf-decorator-client-protocol.h"
#include <wayland-client.h>
#include <string.h>
#include <iostream>
#include <map>

wl_display *display;

std::map<uint32_t, GtkWidget*> view_to_decor;
static void create_new_decoration (void *data,
                               wf_decorator_manager *manager,
                               uint32_t view)
{
    std::cout << "create new decoration" << std::endl;
    auto window = create_deco_window("__wf_decorator:" + std::to_string(view));
    view_to_decor[view] = window;
}

static void title_changed(void *data,
                          wf_decorator_manager *wf_decorator_manager,
                          uint32_t view,
                          const char *new_title)
{
    set_title(view_to_decor[view], new_title);
}


const wf_decorator_manager_listener decorator_listener =
{
    create_new_decoration,
    title_changed
};


void registry_add_object(void*, struct wl_registry *registry, uint32_t name,
        const char *interface, uint32_t)
{
    std::cout << "new registry: " << interface << std::endl;
    if (strcmp(interface, wf_decorator_manager_interface.name) == 0)
    {
        std::cout << "bind it" << std::endl;
        auto decorator_manager =
            (wf_decorator_manager*) wl_registry_bind(registry, name, &wf_decorator_manager_interface, 1u);

        wf_decorator_manager_add_listener(decorator_manager, &decorator_listener, NULL);
    }
}

void registry_remove_object(void*, struct wl_registry*, uint32_t)
{
}

static struct wl_registry_listener registry_listener =
{
    &registry_add_object,
    &registry_remove_object
};


void setup_protocol(GdkDisplay *displ)
{
    auto display = gdk_wayland_display_get_wl_display(displ);

    auto registry = wl_display_get_registry(display);

    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);
    wl_registry_destroy(registry);
}
