#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <gdk/gdkwayland.h>
#include <gtk/gtk.h>
#include <string>

void setup_protocol(GdkDisplay *display);

/* Before mapping (widget.show_all()) the window title must be set EXACTLY as the parameter title */
GtkWidget *create_deco_window(std::string title);

void set_title       (GtkWidget *window, const char *title);
void window_destroyed(GtkWidget *window);


#endif /* end of include guard: PROTOCOL_HPP */
