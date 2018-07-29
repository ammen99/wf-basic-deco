#include "window.hpp"

int main()
{
    auto display = new wayfire_display();

    while (wl_display_dispatch(display->display) != -1)
    {
        test
        // blank
    }
    delete display;
}
