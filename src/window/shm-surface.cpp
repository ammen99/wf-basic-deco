#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include <wayland-client.h>
#include <wayland-client-protocol.h>

#include <cairo.h>

#include "window.hpp"
#include "shm-surface.hpp"
#include "shm-util.cpp"

void wf_shm_pool::init_pool(size_t size)
{
    current_size = 0;
    data = NULL;

    std::cout << "init pool" << std::endl;
    this->pool = make_shm_pool(shm, size, &data);
    if (!pool)
        return;

    current_size = size;
}

void* wf_shm_pool::request(size_t size)
{
    std::cout << "request " << size << " now is " << current_size << std::endl;
    if (current_size < size)
    {
        if (pool)
            fini_pool();

        init_pool(size);
    }

    return data;
}

void wf_shm_pool::fini_pool()
{
    munmap(data, current_size);
    wl_shm_pool_destroy(pool);
}

wf_shm_pool::wf_shm_pool(wl_shm *shm)
{
    this->shm = shm;
}

wf_shm_pool::~wf_shm_pool()
{
    fini_pool();
}

#define target_fmt CAIRO_FORMAT_ARGB32

inline int data_length_for_shm_surface(int width, int height)
{
    return cairo_format_stride_for_width(target_fmt, width) * height;
}

void cairo_surface_destructor(void *p)
{
    auto buffer = (wl_buffer*) p;
    wl_buffer_destroy(buffer);
}

cairo_surface_t* wf_shm_pool::create_cairo_surface(int width, int height)
{
    cairo_surface_t *surface;

    int stride = cairo_format_stride_for_width (target_fmt, width);
    int length = stride * height;
    void *map = request(length);

    if (!map)
        return NULL;

    surface = cairo_image_surface_create_for_data ((unsigned char*) map, target_fmt, width, height, stride);
    auto buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    cairo_surface_set_user_data(surface, &shm_surface_data_key, buffer, cairo_surface_destructor);

    return surface;
}

void wayfire_window::damage_commit()
{
    wl_surface_attach(surface, get_buffer_from_cairo_surface(cairo_surface),0,0);
    wl_surface_damage(surface, 0, 0, width, height);
    wl_surface_commit(surface);
}
