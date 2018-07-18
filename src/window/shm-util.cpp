int set_cloexec_or_close(int fd)
{
	long flags;

	if (fd == -1)
		return -1;

	flags = fcntl(fd, F_GETFD);
	if (flags == -1)
		goto err;

	if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
		goto err;

	return fd;

err:
	close(fd);
	return -1;
}

int create_tmpfile_cloexec(char *tmpname)
{
	int fd;
	fd = mkstemp(tmpname);
	if (fd >= 0)
    {
		fd = set_cloexec_or_close(fd);
		unlink(tmpname);
	}

	return fd;
}

int os_create_anonymous_file(off_t size)
{
	static const char templ[] = "/wayfire-shared-XXXXXX";
	const char *path;
	char *name;
	int fd;
	int ret;

	path = getenv("XDG_RUNTIME_DIR");
	if (!path)
    {
		errno = ENOENT;
		return -1;
	}

    name = new char[strlen(path) + sizeof(templ)];

	strcpy(name, path);
	strcat(name, templ);

	fd = create_tmpfile_cloexec(name);

    delete[] name;

	if (fd < 0)
		return -1;

	ret = posix_fallocate(fd, 0, size);
	if (ret != 0)
    {
		close(fd);
		errno = ret;
		return -1;
	}
	return fd;
}


wl_shm_pool* make_shm_pool(wl_shm *shm, int size, void **data)
{
    wl_shm_pool *pool;
    int fd;

    fd = os_create_anonymous_file(size);
    if (fd < 0) {
        std::cerr << "creating a buffer file for " << size << " failed" << std::endl;
        return NULL;
    }

    *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (*data == MAP_FAILED) {
        std::cerr << "mmap failed" << std::endl;
        close(fd);
        return NULL;
    }

    pool = wl_shm_create_pool(shm, fd, size);

    close(fd);

    return pool;
}

const cairo_user_data_key_t shm_surface_data_key = {0};

wl_buffer* get_buffer_from_cairo_surface(cairo_surface_t *surface)
{
	auto data = static_cast<wl_buffer*>
        (cairo_surface_get_user_data(surface, &shm_surface_data_key));

    return data;
}
