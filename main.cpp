#include "protocol.hpp"
GtkApplication *app;


static void
activate (GtkApplication* app,
          gpointer        user_data)
{
    GdkDisplay* display = gdk_display_get_default();
    setup_protocol(display);

    g_application_hold(G_APPLICATION(app));

  //  gdk_wayland_window_get_wl_surface(gtk_widget_get_window())
}

GtkWidget *create_deco_window(std::string title)
{
    GtkWidget *window;
    window = gtk_application_window_new (app);
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 300);
    gtk_window_set_title(GTK_WINDOW(window), title.c_str());
    gtk_widget_show_all(window);

    return window;
}

void set_title(GtkWidget *window, const char *title)
{
    gtk_window_set_title(GTK_WINDOW(window), title);
}

int
main (int argc, char **argv)
{
    int status;

    app = gtk_application_new ("org.wf.sample-decorator", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}
