#include "window.hpp"
#include "shm-surface.hpp"
#include <cstring>
#include <algorithm>
#include <map>
#include <wayland-cursor.h>
#include <unistd.h>

#include "input.hpp"

static void handle_wl_output_geometry(void *, struct wl_output *, int32_t, int32_t, int32_t, int32_t, int32_t, const char*, const char*, int32_t) { }
static void handle_wl_output_mode(void *, struct wl_output *, uint32_t , int32_t, int32_t, int32_t) { }
static void handle_wl_output_done(void *, struct wl_output *) { }
static void handle_wl_output_scale(void *, struct wl_output *, int32_t) { }

const wl_output_listener output_listener =
{
    handle_wl_output_geometry,
    handle_wl_output_mode,
    handle_wl_output_done,
    handle_wl_output_scale
};

void handle_xdg_wm_base_ping(void*, xdg_wm_base *wm_base, uint32_t serial)
{
    xdg_wm_base_pong(wm_base, serial);
}

struct xdg_wm_base_listener  wm_base_listener {
    handle_xdg_wm_base_ping
};

// listeners
void registry_add_object(void *data, struct wl_registry *registry, uint32_t name,
        const char *interface, uint32_t version)
{
    auto display = (wayfire_display*) data;

    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        display->compositor = (wl_compositor*) wl_registry_bind(registry, name,
                                                                &wl_compositor_interface,
                                                                std::min(version, 3u));
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        display->wm_base = (xdg_wm_base*) wl_registry_bind(registry, name, &xdg_wm_base_interface, std::min(version, 1u));
        xdg_wm_base_add_listener(display->wm_base, &wm_base_listener, NULL);
    }
    else if (strcmp(interface, wl_seat_interface.name) == 0 && display->seat == NULL)
    {
        display->seat = (wl_seat*) wl_registry_bind(registry, name, &wl_seat_interface, std::min(version, 2u));

        auto pointer = wl_seat_get_pointer(display->seat);
        auto touch   = wl_seat_get_touch(display->seat);

        if (pointer)
            wl_pointer_add_listener(pointer, &pointer_listener, NULL);

        if (touch)
            wl_touch_add_listener(touch, &touch_listener, NULL);
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
        display->shm = (wl_shm*) wl_registry_bind(registry, name, &wl_shm_interface, std::min(version, 1u));
    }
    else if (strcmp(interface, wl_output_interface.name) == 0)
    {
        /* XXX: scaling? */
        std::cout << "bind wl_output" << std::endl;
        (void) (wl_output*) wl_registry_bind(registry, name, &wl_output_interface,
                                                    std::min(version, 1u));

    } else if (strcmp(interface, wf_decorator_manager_interface.name) == 0)
    {
        display->decorator_manager =
            (wf_decorator_manager*) wl_registry_bind(registry, name, &wf_decorator_manager_interface, 1u);

        wf_decorator_manager_add_listener(display->decorator_manager, &wf_decorator_manager_impl, display);
    }
}

void registry_remove_object(void *, struct wl_registry *, uint32_t) { }
static struct wl_registry_listener registry_listener =
{
    &registry_add_object,
    &registry_remove_object
};

/* wayfire_display implementation */
wayfire_display::wayfire_display()
{
    display = wl_display_connect(NULL);

    if (!display)
    {
        std::cerr << "Failed to connect to display!" << std::endl;
        std::exit(-1);
    }

    wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, this);
    wl_display_roundtrip(display);

    if (!load_cursor())
    {
        std::cerr << "Failed to load cursor!" << std::endl;
        std::exit(-1);
    }
}

wayfire_display::~wayfire_display()
{
    // TODO: we should also fix up all kinds of shells,
    // registry, etc. here
    wl_display_disconnect(display);
}

bool wayfire_display::load_cursor()
{
    auto cursor_theme = wl_cursor_theme_load(NULL, 16, shm);
    if (!cursor_theme) {
        std::cout << "failed to load cursor theme" << std::endl;
        return false;
    }

    const char* alternatives[] = {
        "left_ptr", "default",
        "top_left_arrow", "left-arrow"
    };

    cursor = NULL;
    for (int i = 0; i < 4 && !cursor; i++)
        cursor = wl_cursor_theme_get_cursor(cursor_theme, alternatives[i]);

    cursor_surface = wl_compositor_create_surface(compositor);
    if (!cursor || !cursor_surface) {
        std::cout << "failed to load cursor" << std::endl;
        return false;
    }

    return true;
}

void wayfire_display::show_default_cursor(uint32_t serial)
{
    auto image = cursor->images[0];
    auto buffer = wl_cursor_image_get_buffer(image);

    wl_surface_attach(cursor_surface, buffer, 0, 0);
    wl_surface_damage(cursor_surface, 0, 0, image->width, image->height);
    wl_surface_commit(cursor_surface);

    auto pointer = wl_seat_get_pointer(seat);
    if (pointer)
        wl_pointer_set_cursor(pointer, serial, cursor_surface, image->hotspot_x, image->hotspot_y);
}

/* wayfire_window impl */
static void xdg_surface_handle_configure(void *data, xdg_surface *xdg_surface, uint32_t serial)
{
    auto window = (wayfire_window*) data;
	xdg_surface_ack_configure(xdg_surface, serial);

    if (window->pending_width != window->width || window->pending_height != window->height)
        window->resize_request(window->pending_width, window->pending_height);
}

static const struct xdg_surface_listener xdg_surface_impl = {
	.configure = xdg_surface_handle_configure,
};

static void xdg_toplevel_handle_configure(void *data, xdg_toplevel *,
                                          int32_t w, int32_t h, wl_array*)
{
    auto window = (wayfire_window*) data;
	if (w == 0 || h == 0)
		return;

    window->pending_width = w;
    window->pending_height = h;
}

static void xdg_toplevel_handle_close(void *data, xdg_toplevel *)
{
    auto window = (wayfire_window*) data;
    window->close_request();
}

static const struct xdg_toplevel_listener xdg_toplevel_impl = {
	.configure = xdg_toplevel_handle_configure,
	.close = xdg_toplevel_handle_close,
};

wayfire_window::wayfire_window(wayfire_display *display, std::string title)
{
    this->pool = NULL;
    this->display = display;

    width = height = 0;
	surface = wl_compositor_create_surface(display->compositor);
    wl_surface_set_user_data(surface, this);

    xsurface = xdg_wm_base_get_xdg_surface(display->wm_base, surface);
    xdg_surface_add_listener(xsurface, &xdg_surface_impl, this);

    xtoplevel = xdg_surface_get_toplevel(xsurface);
    xdg_toplevel_add_listener(xtoplevel, &xdg_toplevel_impl, this);

    xdg_toplevel_set_title(xtoplevel, title.c_str());
    wl_surface_commit(surface);
}

wayfire_window::~wayfire_window()
{
    if (current_pointer_window == this)
        current_pointer_window = nullptr;

    xdg_toplevel_destroy(xtoplevel);
    xdg_surface_destroy(xsurface);
    wl_surface_destroy(surface);
    cairo_surface_destroy(cairo_surface);
}

void wayfire_window::resize(int width, int height)
{
    if (!pool)
        pool = new wf_shm_pool(display->shm);

    if (cairo_surface)
        cairo_surface_destroy(cairo_surface);
    cairo_surface = pool->create_cairo_surface(width, height);

    this->width = width;
    this->height = height;
}

void wayfire_window::set_scale(int scale)
{
    this->scale = scale;
    wl_surface_set_buffer_scale(surface, scale);
}

/* utility functions */
void render_rounded_rectangle(cairo_t *cr, int x, int y, int width, int height,
        double radius, double r, double g, double b, double a)
{
    double degrees = M_PI / 180.0;

    cairo_new_sub_path (cr);
    cairo_arc (cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc (cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);
    cairo_arc (cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);
    cairo_arc (cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path (cr);

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_fill_preserve(cr);
}
