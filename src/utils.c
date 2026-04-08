#include <fcntl.h>
#include "../include/utils.h"

// Met le descripteur en mode non bloquant
void set_nonblocking(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK); // Ajoute le flag O_NONBLOCK
}
