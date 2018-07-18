#ifndef SHM_SURFACE_HPP
#define SHM_SURFACE_HPP

#include "window.hpp"

struct wf_shm_pool
{
    size_t current_size = 0;
    size_t held_size;

    wl_shm *shm;
    wl_shm_pool *pool = NULL;
    void *data;

    wf_shm_pool() = delete;
    wf_shm_pool(wl_shm *shm);
    ~wf_shm_pool();

    /* frees any previously held data */
    void *request(size_t size);

    cairo_surface_t *create_cairo_surface(int width, int height);

    void init_pool(size_t size);
    void fini_pool();
};

#endif /* end of include guard: SHM_SURFACE_HPP */
