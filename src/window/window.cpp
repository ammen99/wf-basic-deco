#include "window.hpp"
#include "shm-surface.hpp"

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
