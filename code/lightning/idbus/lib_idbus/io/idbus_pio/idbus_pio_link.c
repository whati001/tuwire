#include "idbus.h"
#include "idbus_pio.h"

static inline int map_io_layer_return_code(int ret)
{
    switch (ret)
    {
    case IDBUS_PIO_OK:
        return IDBUS_OK;
    case IDBUS_PIO_TIMEOUT:
        return IDBUS_ERR_TIMEOUT;
    default:
        return IDBUS_ERR;
    }
}

int idbus_open_hifive(int *fd, void *param)
{
    return map_io_layer_return_code(open_idbus_pio(fd, HIFIVE, (int)param));
}

int idbus_open_tristar(int *fd, void *param)
{
    return map_io_layer_return_code(open_idbus_pio(fd, TRISTAR, (int)param));
}

int idbus_read(int fd, idbus_msg_t *msg)
{
    return map_io_layer_return_code(read_idbus_pio(fd, (idbus_msg_t *)msg));
}

int idbus_write(int fd, idbus_msg_t *msg)
{
    return map_io_layer_return_code(write_idbus_pio(fd, (idbus_msg_t *)msg));
}

int idbus_reset(int fd)
{
    return map_io_layer_return_code(idbus_reset_pio(fd));
}

int idbus_close(int fd)
{
    return map_io_layer_return_code(close_idbus_pio(fd));
}

int idbus_init()
{
    return map_io_layer_return_code(IDBUS_OK);
}
